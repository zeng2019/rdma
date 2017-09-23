/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#undef PGO_TRAINING
#define PATH_TO_PGO_CONFIG "path_to_pgo_config"

#include <iostream>
#include <fstream>
#include <time.h> 
#include "ns3/core-module.h"
#include "ns3/qbb-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/broadcom-node.h"
#include "ns3/packet.h"
#include "ns3/error-model.h"
#include "ns3/qbb-net-device.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("GENERIC_SIMULATION");

bool qbb_enabled = true, enable_timely = false, enable_qcn = true, use_dynamic_pfc_threshold = true, packet_level_ecmp = false, flow_level_ecmp = false;
uint32_t packet_payload_size = 1000, l2_chunk_size = 0, l2_ack_interval = 0;
double weight_rtt_diff, pause_time = 5, simulator_stop_time = 3.01, app_start_time = 1.0, app_stop_time = 9.0;
std::string data_rate, link_delay, topology_file, flow_file, tcp_flow_file, trace_file, trace_output_file;
bool used_port[65536] = { 0 };

double cnp_interval = 50, alpha_resume_interval = 55, rp_timer, dctcp_gain = 1 / 16, np_sampling_interval = 0, pmax = 1;
uint32_t byte_counter, fast_recovery_times = 5, kmax = 60, kmin = 60;
std::string rate_ai, rate_hai;

uint64_t rtt_low, rtt_high, rtt_num, rtt_noise = 0, min_rtt, step, rate_every_flow, rtt_ref, init_rate;

bool clamp_target_rate = false, clamp_target_rate_after_timer = false, send_in_chunks = true, l2_wait_for_ack = false, l2_back_to_zero = false, l2_test_read = false;
double error_rate_per_link = 0.0;

ofstream ofs_throughput0, ofs_throughput1, ofs_rtt1, ofs_throughput2, ofs_rtt2;
int num1 = 0, num2 = 0, num3 = 0;
double cal_rate_interval = 0.1, weight_bida; //计算速率的间隔
uint16_t cal_rate;

/*
void addName(NodeContainer nodes, string str, uint32_t cnt) {
for (uint32_t i = 0; i < cnt; i++)
Names::Add(str.append(cnt+""), nodes.Get(i));
}
*/
int main(int argc, char *argv[])
{

	clock_t begint, endt;
	begint = clock();
#ifndef PGO_TRAINING
	if (argc > 1)
#else
	if (true)
#endif
	{
		//Read the configuration file
		std::ifstream conf;
#ifndef PGO_TRAINING
		conf.open(argv[1]);
#else
		conf.open(PATH_TO_PGO_CONFIG);
#endif
		while (!conf.eof())
		{
			std::string key;
			conf >> key;

			//std::cout << conf.cur << "\n";
			if (key.compare("RATE_EVERY_FLOW") == 0) {
				uint64_t v;
				conf >> v;
				rate_every_flow = v;
				cout << "RATE_EVERY_FLOW\t\t\t" << rate_every_flow << "\n";
			}
			else if (key.compare("INIT_RATE") == 0) {
				uint64_t v;
				conf >> v;
				init_rate = v;
				cout << "INIT_RATE\t\t\t" << init_rate << "\n";
			}
			else if (key.compare("MIN_RTT") == 0) {
				uint64_t v;
				conf >> v;
				min_rtt = v;
				cout << "MIN_RTT\t\t\t" << v << "\n";
			}
			else if (key.compare("QBB_ENABLED") == 0) {
				bool v;
				conf >> v;
				qbb_enabled = v;
				cout << "QBB_ENABLED\t\t\t" << v << "\n";
			}
			else if (key.compare("RTT_NOISE") == 0) {
				uint64_t v;
				conf >> v;
				rtt_noise = v;
				cout << "RTT_NOISE\t\t\t" << v << "\n";
			}
			else if (key.compare("WEIGHT_BIDA") == 0) {
				double v;
				conf >> v;
				weight_bida = v;
				cout << "WEIGHT_BIDA\t\t\t" << v << "\n";
			}
			else if (key.compare("RTT_NUM") == 0) {
				uint64_t v;
				conf >> v;
				rtt_num = v;
				cout << "RTT_NUM\t\t\t" << v << "\n";
			}
			else if (key.compare("CAL_RATE_INTERVAL") == 0) {
				double v;
				conf >> v;
				cal_rate_interval = v;
				cout << "CAL_RATE_INTERVAL\t\t\t" << v << "\n";
			}
			else if (key.compare("ENABLE_TIMELY") == 0) {
				uint32_t v;
				conf >> v;
				enable_timely = v;
				if (enable_timely)
					cout << "ENABLE_TIMELY\t\t\t" << "Yes" << "\n";
				else
					cout << "ENABLE_TIMELY\t\t\t" << "No" << "\n";
			}
			else if (key.compare("RTT_HIGH") == 0) {
				uint64_t v;
				conf >> v;
				rtt_high = v;
				cout << "RTT_HIGH\t\t\t" << rtt_high << "\n";
			}
			else if (key.compare("RTT_LOW") == 0) {
				uint64_t v;
				conf >> v;
				rtt_low = v;
				cout << "RTT_LOW\t\t\t" << rtt_low << "\n";
			}
			else if (key.compare("WEIGHT_RTT_DIFF") == 0) {
				double v;
				conf >> v;
				weight_rtt_diff = v;
				cout << "WEIGHT_RTT_DIFF\t\t\t" << weight_rtt_diff << "\n";
			}
			else if (key.compare("STEP") == 0) {
				uint64_t v;
				conf >> v;
				step = v;
				cout << "ADDITIVE INCREASE STEP\t\t\t" << step << "\n";
			}
			else if (key.compare("RTT_REF") == 0) {
				uint64_t v;
				conf >> v;
				rtt_ref = v;
				cout << "REFERENCE RTT\t\t\t" << rtt_ref << "\n";
			}
			else if (key.compare("ENABLE_QCN") == 0)
			{
				uint32_t v;
				conf >> v;
				enable_qcn = v;
				if (enable_qcn)
					std::cout << "ENABLE_QCN\t\t\t" << "Yes" << "\n";
				else
					std::cout << "ENABLE_QCN\t\t\t" << "No" << "\n";
			}
			else if (key.compare("USE_DYNAMIC_PFC_THRESHOLD") == 0)
			{
				uint32_t v;
				conf >> v;
				use_dynamic_pfc_threshold = v;
				if (use_dynamic_pfc_threshold)
					std::cout << "USE_DYNAMIC_PFC_THRESHOLD\t" << "Yes" << "\n";
				else
					std::cout << "USE_DYNAMIC_PFC_THRESHOLD\t" << "No" << "\n";
			}
			else if (key.compare("CLAMP_TARGET_RATE") == 0)
			{
				uint32_t v;
				conf >> v;
				clamp_target_rate = v;
				if (clamp_target_rate)
					std::cout << "CLAMP_TARGET_RATE\t\t" << "Yes" << "\n";
				else
					std::cout << "CLAMP_TARGET_RATE\t\t" << "No" << "\n";
			}
			else if (key.compare("CLAMP_TARGET_RATE_AFTER_TIMER") == 0)
			{
				uint32_t v;
				conf >> v;
				clamp_target_rate_after_timer = v;
				if (clamp_target_rate_after_timer)
					std::cout << "CLAMP_TARGET_RATE_AFTER_TIMER\t" << "Yes" << "\n";
				else
					std::cout << "CLAMP_TARGET_RATE_AFTER_TIMER\t" << "No" << "\n";
			}
			else if (key.compare("PACKET_LEVEL_ECMP") == 0)
			{
				uint32_t v;
				conf >> v;
				packet_level_ecmp = v;
				if (packet_level_ecmp)
					std::cout << "PACKET_LEVEL_ECMP\t\t" << "Yes" << "\n";
				else
					std::cout << "PACKET_LEVEL_ECMP\t\t" << "No" << "\n";
			}
			else if (key.compare("FLOW_LEVEL_ECMP") == 0)
			{
				uint32_t v;
				conf >> v;
				flow_level_ecmp = v;
				if (flow_level_ecmp)
					std::cout << "FLOW_LEVEL_ECMP\t\t\t" << "Yes" << "\n";
				else
					std::cout << "FLOW_LEVEL_ECMP\t\t\t" << "No" << "\n";
			}
			else if (key.compare("PAUSE_TIME") == 0)
			{
				double v;
				conf >> v;
				pause_time = v;
				std::cout << "PAUSE_TIME\t\t\t" << pause_time << "\n";
			}
			else if (key.compare("DATA_RATE") == 0)
			{
				std::string v;
				conf >> v;
				data_rate = v;
				std::cout << "DATA_RATE\t\t\t" << data_rate << "\n";
			}
			else if (key.compare("LINK_DELAY") == 0)
			{
				std::string v;
				conf >> v;
				link_delay = v;
				std::cout << "LINK_DELAY\t\t\t" << link_delay << "\n";
			}
			else if (key.compare("PACKET_PAYLOAD_SIZE") == 0)
			{
				uint32_t v;
				conf >> v;
				packet_payload_size = v;
				std::cout << "PACKET_PAYLOAD_SIZE\t\t" << packet_payload_size << "\n";
			}
			else if (key.compare("L2_CHUNK_SIZE") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_chunk_size = v;
				std::cout << "L2_CHUNK_SIZE\t\t\t" << l2_chunk_size << "\n";
			}
			else if (key.compare("L2_ACK_INTERVAL") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_ack_interval = v;
				std::cout << "L2_ACK_INTERVAL\t\t\t" << l2_ack_interval << "\n";
			}
			else if (key.compare("L2_WAIT_FOR_ACK") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_wait_for_ack = v;
				if (l2_wait_for_ack)
					std::cout << "L2_WAIT_FOR_ACK\t\t\t" << "Yes" << "\n";
				else
					std::cout << "L2_WAIT_FOR_ACK\t\t\t" << "No" << "\n";
			}
			else if (key.compare("L2_BACK_TO_ZERO") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_back_to_zero = v;
				if (l2_back_to_zero)
					std::cout << "L2_BACK_TO_ZERO\t\t\t" << "Yes" << "\n";
				else
					std::cout << "L2_BACK_TO_ZERO\t\t\t" << "No" << "\n";
			}
			else if (key.compare("L2_TEST_READ") == 0)
			{
				uint32_t v;
				conf >> v;
				l2_test_read = v;
				if (l2_test_read)
					std::cout << "L2_TEST_READ\t\t\t" << "Yes" << "\n";
				else
					std::cout << "L2_TEST_READ\t\t\t" << "No" << "\n";
			}
			else if (key.compare("TOPOLOGY_FILE") == 0)
			{
				std::string v;
				conf >> v;
				topology_file = v;
				std::cout << "TOPOLOGY_FILE\t\t\t" << topology_file << "\n";
			}
			else if (key.compare("FLOW_FILE") == 0)
			{
				std::string v;
				conf >> v;
				flow_file = v;
				std::cout << "FLOW_FILE\t\t\t" << flow_file << "\n";
			}
			else if (key.compare("TCP_FLOW_FILE") == 0)
			{
				std::string v;
				conf >> v;
				tcp_flow_file = v;
				std::cout << "TCP_FLOW_FILE\t\t\t" << tcp_flow_file << "\n";
			}
			else if (key.compare("TRACE_FILE") == 0)
			{
				std::string v;
				conf >> v;
				trace_file = v;
				std::cout << "TRACE_FILE\t\t\t" << trace_file << "\n";
			}
			else if (key.compare("TRACE_OUTPUT_FILE") == 0)
			{
				std::string v;
				conf >> v;
				trace_output_file = v;
				if (argc > 2)
				{
					trace_output_file = trace_output_file + std::string(argv[2]);
				}
				std::cout << "TRACE_OUTPUT_FILE\t\t" << trace_output_file << "\n";
			}
			else if (key.compare("APP_START_TIME") == 0)
			{
				double v;
				conf >> v;
				app_start_time = v;
				std::cout << "SINK_START_TIME\t\t\t" << app_start_time << "\n";
			}
			else if (key.compare("APP_STOP_TIME") == 0)
			{
				double v;
				conf >> v;
				app_stop_time = v;
				std::cout << "SINK_STOP_TIME\t\t\t" << app_stop_time << "\n";
			}
			else if (key.compare("SIMULATOR_STOP_TIME") == 0)
			{
				double v;
				conf >> v;
				simulator_stop_time = v;
				std::cout << "SIMULATOR_STOP_TIME\t\t" << simulator_stop_time << "\n";
			}
			else if (key.compare("CNP_INTERVAL") == 0)
			{
				double v;
				conf >> v;
				cnp_interval = v;
				std::cout << "CNP_INTERVAL\t\t\t" << cnp_interval << "\n";
			}
			else if (key.compare("ALPHA_RESUME_INTERVAL") == 0)
			{
				double v;
				conf >> v;
				alpha_resume_interval = v;
				std::cout << "ALPHA_RESUME_INTERVAL\t\t" << alpha_resume_interval << "\n";
			}
			else if (key.compare("RP_TIMER") == 0)
			{
				double v;
				conf >> v;
				rp_timer = v;
				std::cout << "RP_TIMER\t\t\t" << rp_timer << "\n";
			}
			else if (key.compare("BYTE_COUNTER") == 0)
			{
				uint32_t v;
				conf >> v;
				byte_counter = v;
				std::cout << "BYTE_COUNTER\t\t\t" << byte_counter << "\n";
			}
			else if (key.compare("KMAX") == 0)
			{
				uint32_t v;
				conf >> v;
				kmax = v;
				std::cout << "KMAX\t\t\t\t" << kmax << "\n";
			}
			else if (key.compare("KMIN") == 0)
			{
				uint32_t v;
				conf >> v;
				kmin = v;
				std::cout << "KMIN\t\t\t\t" << kmin << "\n";
			}
			else if (key.compare("PMAX") == 0)
			{
				double v;
				conf >> v;
				pmax = v;
				std::cout << "PMAX\t\t\t\t" << pmax << "\n";
			}
			else if (key.compare("DCTCP_GAIN") == 0)
			{
				double v;
				conf >> v;
				dctcp_gain = v;
				std::cout << "DCTCP_GAIN\t\t\t" << dctcp_gain << "\n";
			}
			else if (key.compare("FAST_RECOVERY_TIMES") == 0)
			{
				uint32_t v;
				conf >> v;
				fast_recovery_times = v;
				std::cout << "FAST_RECOVERY_TIMES\t\t" << fast_recovery_times << "\n";
			}
			else if (key.compare("RATE_AI") == 0)
			{
				std::string v;
				conf >> v;
				rate_ai = v;
				std::cout << "RATE_AI\t\t\t\t" << rate_ai << "\n";
			}
			else if (key.compare("RATE_HAI") == 0)
			{
				std::string v;
				conf >> v;
				rate_hai = v;
				std::cout << "RATE_HAI\t\t\t" << rate_hai << "\n";
			}
			else if (key.compare("NP_SAMPLING_INTERVAL") == 0)
			{
				double v;
				conf >> v;
				np_sampling_interval = v;
				std::cout << "NP_SAMPLING_INTERVAL\t\t" << np_sampling_interval << "\n";
			}
			else if (key.compare("SEND_IN_CHUNKS") == 0)
			{
				uint32_t v;
				conf >> v;
				send_in_chunks = v;
				if (send_in_chunks)
				{
					std::cout << "SEND_IN_CHUNKS\t\t\t" << "Yes" << "\n";
					std::cout << "WARNING: deprecated and not tested. Please consider using L2_WAIT_FOR_ACK";
				}
				else
					std::cout << "SEND_IN_CHUNKS\t\t\t" << "No" << "\n";
			}
			else if (key.compare("ERROR_RATE_PER_LINK") == 0)
			{
				double v;
				conf >> v;
				error_rate_per_link = v;
				std::cout << "ERROR_RATE_PER_LINK\t\t" << error_rate_per_link << "\n";
			}
			else if (key.compare("APP_CAL_RATE") == 0)
			{
				double v;
				conf >> v;
				cal_rate = v;
				std::cout << "APP_CAL_RATE\t\t" << cal_rate << "\n";
			}
			fflush(stdout);
		}
		conf.close();
	}
	else
	{
		std::cout << "Error: require a config file\n";
		fflush(stdout);
		return 1;
	}


	bool dynamicth = use_dynamic_pfc_threshold;

	NS_ASSERT(packet_level_ecmp + flow_level_ecmp < 2); //packet level ecmp and flow level ecmp are exclusive

	Config::SetDefault("ns3::QbbNetDevice::InitRate", UintegerValue(init_rate));
	Config::SetDefault("ns3::QbbNetDevice::RateEveryFlow", UintegerValue(rate_every_flow));
	Config::SetDefault("ns3::QbbNetDevice::MinRtt", UintegerValue(min_rtt));
	Config::SetDefault("ns3::QbbNetDevice::QbbEnabled", BooleanValue(qbb_enabled));
	Config::SetDefault("ns3::QbbNetDevice::RttNoise", UintegerValue(rtt_noise));
	Config::SetDefault("ns3::QbbNetDevice::RttNum", UintegerValue(rtt_num));
	Config::SetDefault("ns3::QbbNetDevice::WeightBida", DoubleValue(weight_bida));
	Config::SetDefault("ns3::QbbNetDevice::TimelyEnabled", BooleanValue(enable_timely));
	Config::SetDefault("ns3::QbbNetDevice::RttHigh", UintegerValue(rtt_high));
	Config::SetDefault("ns3::QbbNetDevice::RttLow", UintegerValue(rtt_low));
	Config::SetDefault("ns3::QbbNetDevice::WeightRttDiff", DoubleValue(weight_rtt_diff));
	Config::SetDefault("ns3::QbbNetDevice::RttRef", UintegerValue(rtt_ref));
	Config::SetDefault("ns3::QbbNetDevice::Step", UintegerValue(step));
	Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(packet_level_ecmp));
	Config::SetDefault("ns3::Ipv4GlobalRouting::FlowEcmpRouting", BooleanValue(flow_level_ecmp));
	Config::SetDefault("ns3::QbbNetDevice::PauseTime", UintegerValue(pause_time));
	Config::SetDefault("ns3::QbbNetDevice::QcnEnabled", BooleanValue(enable_qcn));
	Config::SetDefault("ns3::QbbNetDevice::DynamicThreshold", BooleanValue(dynamicth));
	Config::SetDefault("ns3::QbbNetDevice::ClampTargetRate", BooleanValue(clamp_target_rate));
	Config::SetDefault("ns3::QbbNetDevice::ClampTargetRateAfterTimeInc", BooleanValue(clamp_target_rate_after_timer));
	Config::SetDefault("ns3::QbbNetDevice::CNPInterval", DoubleValue(cnp_interval));
	Config::SetDefault("ns3::QbbNetDevice::NPSamplingInterval", DoubleValue(np_sampling_interval));
	Config::SetDefault("ns3::QbbNetDevice::AlphaResumInterval", DoubleValue(alpha_resume_interval));
	Config::SetDefault("ns3::QbbNetDevice::RPTimer", DoubleValue(rp_timer));
	Config::SetDefault("ns3::QbbNetDevice::ByteCounter", UintegerValue(byte_counter));
	Config::SetDefault("ns3::QbbNetDevice::FastRecoveryTimes", UintegerValue(fast_recovery_times));
	Config::SetDefault("ns3::QbbNetDevice::DCTCPGain", DoubleValue(dctcp_gain));
	Config::SetDefault("ns3::QbbNetDevice::RateAI", DataRateValue(DataRate(rate_ai)));
	Config::SetDefault("ns3::QbbNetDevice::RateHAI", DataRateValue(DataRate(rate_hai)));
	Config::SetDefault("ns3::QbbNetDevice::L2BackToZero", BooleanValue(l2_back_to_zero));
	Config::SetDefault("ns3::QbbNetDevice::L2TestRead", BooleanValue(l2_test_read));
	Config::SetDefault("ns3::QbbNetDevice::L2ChunkSize", UintegerValue(l2_chunk_size));
	Config::SetDefault("ns3::QbbNetDevice::L2AckInterval", UintegerValue(l2_ack_interval));
	Config::SetDefault("ns3::QbbNetDevice::L2WaitForAck", BooleanValue(l2_wait_for_ack));

	SeedManager::SetSeed(time(NULL));

	std::ifstream topof, flowf, tracef, tcpflowf;
	topof.open(topology_file.c_str());
	flowf.open(flow_file.c_str());
	tracef.open(trace_file.c_str());
	tcpflowf.open(tcp_flow_file.c_str());
	uint32_t node_num, switch_num, link_num, flow_num, trace_num, tcp_flow_num;
	topof >> node_num >> switch_num >> link_num;
	flowf >> flow_num;
	tracef >> trace_num;
	tcpflowf >> tcp_flow_num;


	NodeContainer n;
	n.Create(node_num);
	for (uint32_t i = 0; i < switch_num; i++)
	{
		uint32_t sid;
		topof >> sid;
		n.Get(sid)->SetNodeType(1, dynamicth); //broadcom switch
		n.Get(sid)->m_broadcom->SetMarkingThreshold(kmin, kmax, pmax);
	}


	NS_LOG_INFO("Create nodes.");

	InternetStackHelper internet;
	internet.Install(n);

	NS_LOG_INFO("Create channels.");

	//
	// Explicitly create the channels required by the topology.
	//

	Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
	Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
	rem->SetRandomVariable(uv);
	uv->SetStream(50);
	rem->SetAttribute("ErrorRate", DoubleValue(error_rate_per_link));
	rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
	QbbHelper qbb;
	Ipv4AddressHelper ipv4;
	vector<NetDeviceContainer> devices;
	for (uint32_t i = 0; i < link_num; i++)
	{
		uint32_t src, dst;
		std::string data_rate, link_delay;
		double error_rate;
		topof >> src >> dst >> data_rate >> link_delay >> error_rate;

		qbb.SetDeviceAttribute("DataRate", StringValue(data_rate));
		qbb.SetChannelAttribute("Delay", StringValue(link_delay));

		if (error_rate > 0)
		{
			Ptr<RateErrorModel> rem = CreateObject<RateErrorModel>();
			Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
			rem->SetRandomVariable(uv);
			uv->SetStream(50);
			rem->SetAttribute("ErrorRate", DoubleValue(error_rate));
			rem->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
			qbb.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
		}
		else
		{
			qbb.SetDeviceAttribute("ReceiveErrorModel", PointerValue(rem));
		}

		fflush(stdout);
		NetDeviceContainer d = qbb.Install(n.Get(src), n.Get(dst));
		devices.push_back(d);
		char ipstring[16];
		sprintf(ipstring, "10.%d.%d.0", i / 254 + 1, i % 254 + 1);
		ipv4.SetBase(ipstring, "255.255.255.0");
		ipv4.Assign(d);
	}


	NodeContainer trace_nodes;
	for (uint32_t i = 0; i < trace_num; i++)
	{
		uint32_t nid;
		tracef >> nid;
		trace_nodes = NodeContainer(trace_nodes, n.Get(nid));
	}
	AsciiTraceHelper ascii;
	qbb.EnableAscii(ascii.CreateFileStream(trace_output_file), trace_nodes);

	qbb.EnablePcap("mix/packet/packet", trace_nodes);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	NS_LOG_INFO("Create Applications.");

	uint32_t packetSize = packet_payload_size;
	Time interPacketInterval = Seconds(0.0000005 / 2);

	for (uint32_t i = 0; i < flow_num; i++)
	{
		uint32_t src, dst, pg, maxPacketCount, port;
		double start_time, stop_time;
		while (used_port[port = int(UniformVariable(0, 1).GetValue() * 40000)])
			continue;
		used_port[port] = true;
		flowf >> src >> dst >> pg >> maxPacketCount >> start_time >> stop_time;
		NS_ASSERT(n.Get(src)->GetNodeType() == 0 && n.Get(dst)->GetNodeType() == 0);
		Ptr<Ipv4> ipv4 = n.Get(dst)->GetObject<Ipv4>();
		Ipv4Address serverAddress = ipv4->GetAddress(1, 0).GetLocal(); //GetAddress(0,0) is the loopback 127.0.0.1

		if (send_in_chunks)
		{

			//考虑开不开QCN

			UdpEchoServerHelper server0(port, pg); //Add Priority
			ApplicationContainer apps0s = server0.Install(n.Get(dst));
			apps0s.Start(Seconds(app_start_time));
			apps0s.Stop(Seconds(app_stop_time));
			UdpEchoClientHelper client0(serverAddress, port, pg); //Add Priority
			client0.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
			client0.SetAttribute("Interval", TimeValue(interPacketInterval));
			client0.SetAttribute("PacketSize", UintegerValue(packetSize));
			client0.SetAttribute("ChunkSize", UintegerValue(4000000));  //default 4000000
			ApplicationContainer apps0c = client0.Install(n.Get(src));
			apps0c.Start(Seconds(start_time));
			apps0c.Stop(Seconds(stop_time));

			//	cout << "udpEcho" << endl;

		}
		else
		{
			UdpServerHelper server0(port);
			ApplicationContainer apps0s = server0.Install(n.Get(dst));
			apps0s.Start(Seconds(app_start_time));     //config.txt设置的服务器开始时间
			apps0s.Stop(Seconds(app_stop_time));       //config.txt设置的服务器结束时间
			UdpClientHelper client0(serverAddress, port, pg); //Add Priority
			client0.SetAttribute("CalRate", UintegerValue(cal_rate));
			client0.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
			client0.SetAttribute("Interval", TimeValue(interPacketInterval));
			client0.SetAttribute("PacketSize", UintegerValue(packetSize));
			ApplicationContainer apps0c = client0.Install(n.Get(src));
			apps0c.Start(Seconds(start_time));    //flow.txt配置文件中设置的客户端开始时间
			apps0c.Stop(Seconds(stop_time));      //flow.txt配置文件中设置的客户端结束时间	

												  //	cout << "udp" << endl;

		}

	}

	double start_time, stop_time;
	for (uint32_t i = 0; i < tcp_flow_num; i++)
	{
		uint32_t src, dst, pg, maxPacketCount, port;   //pg似乎没用
		while (used_port[port = int(UniformVariable(0, 1).GetValue() * 40000)])
			continue;
		used_port[port] = true;
		tcpflowf >> src >> dst >> pg >> maxPacketCount >> start_time >> stop_time;
		NS_ASSERT(n.Get(src)->GetNodeType() == 0 && n.Get(dst)->GetNodeType() == 0);
		Ptr<Ipv4> ipv4 = n.Get(dst)->GetObject<Ipv4>();
		Ipv4Address serverAddress = ipv4->GetAddress(1, 0).GetLocal();

		Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
		PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);

		ApplicationContainer sinkApp = sinkHelper.Install(n.Get(dst));
		sinkApp.Start(Seconds(app_start_time));
		sinkApp.Stop(Seconds(app_stop_time));

		BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(serverAddress, port));
		// Set the amount of data to send in bytes.  Zero is unlimited.
		source.SetAttribute("MaxBytes", UintegerValue(maxPacketCount*packet_payload_size));
		ApplicationContainer sourceApps = source.Install(n.Get(src));
		sourceApps.Start(Seconds(start_time));
		sourceApps.Stop(Seconds(stop_time));

		//cout << "tcp" << endl;

	}


	topof.close();
	flowf.close();
	tracef.close();
	tcpflowf.close();

	//
	// Now, do the actual simulation.
	//
	std::cout << "Running Simulation.\n";
	fflush(stdout);
	NS_LOG_INFO("Run Simulation.");

	void traceRate1(int64_t oldValue, int64_t newValue);
	void traceRtt1(int64_t oldValue, int64_t newValue);
	void traceAck1(uint64_t oldValue, uint64_t newValue);
	void traceRate2(int64_t oldValue, int64_t newValue);
	void traceRtt2(int64_t oldValue, int64_t newValue);
	void traceAck2(uint64_t oldValue, uint64_t newValue);
	void traceRate3(int64_t oldValue, int64_t newValue);
	void traceRtt3(int64_t oldValue, int64_t newValue);
	void traceAck3(uint64_t oldValue, uint64_t newValue);
	Ptr<QbbNetDevice> qbbNetDevice0 = DynamicCast<QbbNetDevice>(devices[0].Get(1));
	Ptr<QbbNetDevice> qbbNetDevice1 = DynamicCast<QbbNetDevice>(devices[1].Get(1));
	Ptr<QbbNetDevice> qbbNetDevice2 = DynamicCast<QbbNetDevice>(devices[2].Get(1));
	Ptr<QbbNetDevice> qbbNetDevice3 = DynamicCast<QbbNetDevice>(devices[3].Get(1));

	if (!qbbNetDevice3)
		cout << "NULL" << endl;
	else
		cout << "NOT NULL" << endl;
	//qbbNetDevice1->TraceConnectWithoutContext("Current_Rate", MakeCallback(&traceRate1));  //只是其中某条流的速率
	//qbbNetDevice1->TraceConnectWithoutContext("New_Rtt", MakeCallback(&traceRtt1));
	qbbNetDevice1->TraceConnectWithoutContext("Ack_Cnt", MakeCallback(&traceAck1));
	//qbbNetDevice2->TraceConnectWithoutContext("Current_Rate", MakeCallback(&traceRate2));
	//qbbNetDevice2->TraceConnectWithoutContext("New_Rtt", MakeCallback(&traceRtt2));
	qbbNetDevice2->TraceConnectWithoutContext("Ack_Cnt", MakeCallback(&traceAck2));
	//qbbNetDevice3->TraceConnectWithoutContext("Current_Rate", MakeCallback(&traceRate3));
	//qbbNetDevice3->TraceConnectWithoutContext("New_Rtt", MakeCallback(&traceRtt3));
	//qbbNetDevice3->TraceConnectWithoutContext("Ack_Cnt", MakeCallback(&traceAck3));

	Simulator::Stop(Seconds(simulator_stop_time));

	ofs_throughput0.open("mix/data/throughput0.txt");
	ofs_throughput1.open("mix/data/throughput1.txt");
	ofs_rtt1.open("mix/data/rtt1.txt");
	ofs_throughput2.open("mix/data/throughput2.txt");
	ofs_rtt2.open("mix/data/rtt2.txt");
	if (ofs_throughput0 && ofs_throughput1 && ofs_rtt1 && ofs_throughput2 && ofs_rtt2)
		cout << "文件打开成功" << endl;
	else
		cout << "文件打开失败" << endl;
	void calRate0(Ptr<QbbNetDevice>);
	void calRate1(Ptr<QbbNetDevice>);
	void calRtt1(Ptr<QbbNetDevice>);
	void calRate2(Ptr<QbbNetDevice>);
	void calRtt2(Ptr<QbbNetDevice>);
	Simulator::Schedule(Seconds(2.001), &calRate0, qbbNetDevice0);
	Simulator::Schedule(Seconds(2.001), &calRate1, qbbNetDevice1);
	Simulator::Schedule(Seconds(2.002), &calRtt1, qbbNetDevice1);
	Simulator::Schedule(Seconds(2.003), &calRate2, qbbNetDevice2);
	Simulator::Schedule(Seconds(2.004), &calRtt2, qbbNetDevice2);
	Simulator::Run();
	Simulator::Destroy();
	ofs_throughput1.close();
	ofs_rtt1.close();
	ofs_throughput2.close();
	ofs_rtt2.close();
	ofs_throughput0.close();

	NS_LOG_INFO("Done.");
	cout << "Done!" << endl;
	//cout << "TimeStep: " <<Seconds(1).GetTimeStep() << endl;	
	cout << "共收到ACK: " << num1 << endl;
	cout << "共收到ACK: " << num2 << endl;
	cout << "共收到ACK: " << num3 << endl;
}

uint64_t pre_receive = 0;
uint64_t pre_sent[5] = { 0 };
uint64_t preSent[5][5] = { 0 };   //从1开始
void calRate0(Ptr<QbbNetDevice> qbbNetDevice) {
	if (pre_receive == 0) {
		pre_receive = qbbNetDevice->m_received;
		Simulator::Schedule(Seconds(cal_rate_interval), &calRate0, qbbNetDevice);
		return;
	}
	double received = qbbNetDevice->m_received - pre_receive;
	pre_receive = qbbNetDevice->m_received;
	double rate = received * 8 / (cal_rate_interval * 1000 * 1000 * 1000);
	ofs_throughput0 << Simulator::Now().GetSeconds() - 2 << " " << rate << endl;
	Simulator::Schedule(Seconds(cal_rate_interval), &calRate0, qbbNetDevice);
}

void calRate1(Ptr<QbbNetDevice> qbbNetDevice) {
	if (pre_sent[1] == 0) {
		pre_sent[1] = qbbNetDevice->m_sent;
		Simulator::Schedule(Seconds(cal_rate_interval), &calRate1, qbbNetDevice);
		return;
	}
	uint64_t sent = qbbNetDevice->m_sent - pre_sent[1];
	pre_sent[1] = qbbNetDevice->m_sent;
	double rate = sent * 8 / (cal_rate_interval * 1000 * 1000); // 1024 or 1000?
	ofs_throughput1 << Simulator::Now().GetSeconds() - 2 << " " << rate << " ";  //GetTimeStep() - 2000000
																				 //cout << qbbNetDevice->m_sent << endl;
	for (int i = 1; i <= 4; i++) {
		sent = qbbNetDevice->sent[i] - preSent[1][i];
		preSent[1][i] = qbbNetDevice->sent[i];
		rate = sent * 8 / (cal_rate_interval * 1000 * 1000);
		ofs_throughput1 << rate << " ";
	}
	ofs_throughput1 << endl;
	Simulator::Schedule(Seconds(cal_rate_interval), &calRate1, qbbNetDevice);
}

void calRtt1(Ptr<QbbNetDevice> qbbNetDevice) {
	ofs_rtt1 << Simulator::Now().GetSeconds() - 2 << " "; // << qbbNetDevice->m_rtt / 1000 << " ";  //GetTimeStep() - 2000000  网卡的RTT（微秒）
	ofs_rtt1 << qbbNetDevice->rtt[1] / 1000 << " " << qbbNetDevice->rtt[2] / 1000 << " " << qbbNetDevice->rtt[3] / 1000 << " " << qbbNetDevice->rtt[4] / 1000 << endl; //每条流的RTT
	Simulator::Schedule(Seconds(cal_rate_interval), &calRtt1, qbbNetDevice);
}

void calRate2(Ptr<QbbNetDevice> qbbNetDevice) {
	if (pre_sent[2] == 0) {
		pre_sent[2] = qbbNetDevice->m_sent;
		Simulator::Schedule(Seconds(cal_rate_interval), &calRate2, qbbNetDevice);
		return;
	}
	double sent = qbbNetDevice->m_sent - pre_sent[2];
	pre_sent[2] = qbbNetDevice->m_sent;
	double rate = sent * 8 / (cal_rate_interval * 1000 * 1000); // 1024 or 1000?
	ofs_throughput2 << Simulator::Now().GetSeconds() - 2 << " " << rate << " ";  //GetTimeStep() - 2000000
																				 //cout << qbbNetDevice->m_sent << endl;
	for (int i = 1; i <= 4; i++) {
		sent = qbbNetDevice->sent[i] - preSent[2][i];
		preSent[2][i] = qbbNetDevice->sent[i];
		rate = sent * 8 / (cal_rate_interval * 1000 * 1000);
		ofs_throughput2 << rate << " ";
	}
	ofs_throughput2 << endl;
	Simulator::Schedule(Seconds(cal_rate_interval), &calRate2, qbbNetDevice);
}

void calRtt2(Ptr<QbbNetDevice> qbbNetDevice) {
	ofs_rtt2 << Simulator::Now().GetSeconds() - 2 << " "; // << qbbNetDevice->m_rtt / 1000 << " ";  //GetTimeStep() - 2000000  网卡的RTT（微秒）
	ofs_rtt2 << qbbNetDevice->rtt[1] / 1000 << " " << qbbNetDevice->rtt[2] / 1000 << " " << qbbNetDevice->rtt[3] / 1000 << " " << qbbNetDevice->rtt[4] / 1000 << endl; //每条流的RTT
	Simulator::Schedule(Seconds(cal_rate_interval), &calRtt2, qbbNetDevice);
}

void traceAck1(uint64_t oldValue, uint64_t newValue) {
	num1++;
}

void traceRate2(int64_t oldValue, int64_t newValue) {
	std::cout << "RATE2: " << oldValue << "  " << newValue << endl;
}
void traceRtt2(int64_t oldValue, int64_t newValue) {
	std::cout << "RTT2:  " << oldValue << "  " << newValue << endl;
}
void traceAck2(uint64_t oldValue, uint64_t newValue) {
	num2++;
}

void traceRate3(int64_t oldValue, int64_t newValue) {
	std::cout << "RATE3: " << oldValue << "  " << newValue << endl;
}
void traceRtt3(int64_t oldValue, int64_t newValue) {
	std::cout << "RTT3:  " << oldValue << "  " << newValue << endl;
}
void traceAck3(uint64_t oldValue, uint64_t newValue) {
	num3++;
}