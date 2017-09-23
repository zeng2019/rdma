/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
* Copyright (c) 2006 Georgia Tech Research Corporation, INRIA
*
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
*
* Author: Yibo Zhu <yibzh@microsoft.com>
*/

#define __STDC_LIMIT_MACROS 1
#include <stdint.h>
#include <stdio.h>
#include "ns3/qbb-net-device.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/data-rate.h"
#include "ns3/object-vector.h"
#include "ns3/pause-header.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/assert.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-header.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-channel.h"
#include "ns3/qbb-channel.h"
#include "ns3/random-variable.h"
#include "ns3/flow-id-tag.h"
#include "ns3/qbb-header.h"
#include "ns3/error-model.h"
#include "ns3/cn-header.h"
#include "ns3/ppp-header.h"
#include "ns3/udp-header.h"
#include "ns3/seq-ts-header.h"

#include <iostream>

NS_LOG_COMPONENT_DEFINE("QbbNetDevice");

namespace ns3 {

	NS_OBJECT_ENSURE_REGISTERED(QbbNetDevice);

	TypeId
		QbbNetDevice::GetTypeId(void)
	{
		static TypeId tid = TypeId("ns3::QbbNetDevice")
			.SetParent<PointToPointNetDevice>()
			.AddConstructor<QbbNetDevice>()
			.AddAttribute("QbbEnabled",
				"Enable the generation of PAUSE packet.",
				BooleanValue(true),
				MakeBooleanAccessor(&QbbNetDevice::m_qbbEnabled),
				MakeBooleanChecker())
			.AddAttribute("QcnEnabled",
				"Enable the QCN.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_qcnEnabled),
				MakeBooleanChecker())
			.AddAttribute("DynamicThreshold",
				"Enable dynamic threshold.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_dynamicth),
				MakeBooleanChecker())
			.AddAttribute("ClampTargetRate",
				"Clamp target rate.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_EcnClampTgtRate),
				MakeBooleanChecker())
			.AddAttribute("ClampTargetRateAfterTimeInc",
				"Clamp target rate after timer increase.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_EcnClampTgtRateAfterTimeInc),
				MakeBooleanChecker())
			.AddAttribute("PauseTime",
				"Number of microseconds to pause upon congestion",
				UintegerValue(5),
				MakeUintegerAccessor(&QbbNetDevice::m_pausetime),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("CNPInterval",
				"The interval of generating CNP",
				DoubleValue(50.0),
				MakeDoubleAccessor(&QbbNetDevice::m_qcn_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("AlphaResumInterval",
				"The interval of resuming alpha",
				DoubleValue(55.0),
				MakeDoubleAccessor(&QbbNetDevice::m_alpha_resume_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("RPTimer",
				"The rate increase timer at RP in microseconds",
				DoubleValue(1500.0),
				MakeDoubleAccessor(&QbbNetDevice::m_rpgTimeReset),
				MakeDoubleChecker<double>())
			.AddAttribute("FastRecoveryTimes",
				"The rate increase timer at RP",
				UintegerValue(5),
				MakeUintegerAccessor(&QbbNetDevice::m_rpgThreshold),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("DCTCPGain",
				"Control gain parameter which determines the level of rate decrease",
				DoubleValue(1.0 / 16),
				MakeDoubleAccessor(&QbbNetDevice::m_g),
				MakeDoubleChecker<double>())
			.AddAttribute("MinRate",
				"Minimum rate of a throttled flow",
				DataRateValue(DataRate("100Mb/s")),
				MakeDataRateAccessor(&QbbNetDevice::m_minRate),
				MakeDataRateChecker())
			.AddAttribute("ByteCounter",
				"Byte counter constant for increment process.",
				UintegerValue(150000),
				MakeUintegerAccessor(&QbbNetDevice::m_bc),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("RateAI",
				"Rate increment unit in AI period",
				DataRateValue(DataRate("5Mb/s")),
				MakeDataRateAccessor(&QbbNetDevice::m_rai),
				MakeDataRateChecker())
			.AddAttribute("RateHAI",
				"Rate increment unit in hyperactive AI period",
				DataRateValue(DataRate("50Mb/s")),
				MakeDataRateAccessor(&QbbNetDevice::m_rhai),
				MakeDataRateChecker())
			.AddAttribute("NPSamplingInterval",
				"The QCN NP sampling interval",
				DoubleValue(0.0),
				MakeDoubleAccessor(&QbbNetDevice::m_qcn_np_sampling_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("NACK Generation Interval",
				"The NACK Generation interval",
				DoubleValue(500.0),
				MakeDoubleAccessor(&QbbNetDevice::m_nack_interval),
				MakeDoubleChecker<double>())
			.AddAttribute("L2BackToZero",
				"Layer 2 go back to zero transmission.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_backto0),
				MakeBooleanChecker())
			.AddAttribute("L2TestRead",
				"Layer 2 test read go back 0 but NACK from n.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_testRead),
				MakeBooleanChecker())
			.AddAttribute("L2ChunkSize",
				"Layer 2 chunk size. Disable chunk mode if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&QbbNetDevice::m_chunk),
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("L2AckInterval",
				"Layer 2 Ack intervals. Disable ack if equals to 0.",
				UintegerValue(0),
				MakeUintegerAccessor(&QbbNetDevice::m_ack_interval),   //ÿ�յ�m_ack_interval����������һ��ACK
				MakeUintegerChecker<uint32_t>())
			.AddAttribute("L2WaitForAck",
				"Wait for Ack before sending out next message.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_waitAck),
				MakeBooleanChecker())
			.AddAttribute("L2WaitForAckTimer",
				"Sender's timer of waiting for the ack",
				DoubleValue(500.0),
				MakeDoubleAccessor(&QbbNetDevice::m_waitAckTimer),
				MakeDoubleChecker<double>())
			.AddAttribute("TimelyEnabled",
				"Enable timely.",
				BooleanValue(false),
				MakeBooleanAccessor(&QbbNetDevice::m_timelyEnable),
				MakeBooleanChecker())
			.AddAttribute("RttHigh",
				"RttHigh,used for HAI mode",
				UintegerValue(500000),
				MakeUintegerAccessor(&QbbNetDevice::rtt_hight),
				MakeUintegerChecker<uint64_t>())
			.AddAttribute("RttLow",
				"RttLow,used for HAI mode",
				UintegerValue(50000),
				MakeUintegerAccessor(&QbbNetDevice::rtt_low),
				MakeUintegerChecker<uint64_t>())
			.AddAttribute("Step",
				"Additive increase step",
				UintegerValue(10000000),   //10Mbps
				MakeUintegerAccessor(&QbbNetDevice::step),
				MakeUintegerChecker<uint64_t>())
			.AddAttribute("RttNum",
				"The number of packet in segment with timely!",
				UintegerValue(16),
				MakeUintegerAccessor(&QbbNetDevice::rtt_num),
				MakeUintegerChecker<uint64_t>())
			.AddAttribute("RateEveryFlow",
				"Initial rate of every flow.",
				UintegerValue(500000000),  //500Mbps
				MakeUintegerAccessor(&QbbNetDevice::rate_every_flow),
				MakeUintegerChecker<uint64_t>())
			.AddAttribute("WeightRttDiff",
				"The weight to tune rtt_diff.",
				DoubleValue(0.875),
				MakeDoubleAccessor(&QbbNetDevice::weight_rtt_diff),
				MakeDoubleChecker<double>())
			.AddAttribute("WeightBida",
				"WeightBida",
				DoubleValue(0.8),
				MakeDoubleAccessor(&QbbNetDevice::weight_bida),
				MakeDoubleChecker<double>())
			.AddAttribute("RttNoise",
				"RttNoise added to rtt",
				UintegerValue(0),  //16KB
				MakeUintegerAccessor(&QbbNetDevice::rtt_noise),
				MakeUintegerChecker<uint64_t>())
			.AddAttribute("RttRef",
				"Reference Rtt",
				UintegerValue(300000),
				MakeUintegerAccessor(&QbbNetDevice::rtt_ref),
				MakeUintegerChecker<uint64_t>())
			.AddAttribute("MinRtt",
				"MinRtt",
				UintegerValue(1800000),  //1800΢��
				MakeUintegerAccessor(&QbbNetDevice::m_min_rtt),
				MakeUintegerChecker<uint64_t>())
			.AddAttribute("InitRate",
				"initial rate",
				UintegerValue(10000000000),  //1800΢��
				MakeUintegerAccessor(&QbbNetDevice::initRate),
				MakeUintegerChecker<uint64_t>())
			.AddTraceSource("Current_Rate",
				"NIC current rate",
				MakeTraceSourceAccessor(&QbbNetDevice::rate))
			.AddTraceSource("New_Rtt",
				"Current rtt",
				MakeTraceSourceAccessor(&QbbNetDevice::new_rtt))
			.AddTraceSource("Ack_Cnt",
				"Received ack cnt",
				MakeTraceSourceAccessor(&QbbNetDevice::ack_cnt))
			;

		return tid;
	}

	QbbNetDevice::QbbNetDevice()
	{
		NS_LOG_FUNCTION(this);
		m_ecn_source = new std::vector<ECNAccount>;
		for (uint32_t i = 0; i < qCnt; i++)
		{
			m_paused[i] = false;
		}
		m_rtt = 0;
		rtt[0] = 0;
		rtt[1] = 0;
		rtt[2] = 0;
		rtt[3] = 0;
		m_qcn_np_sampling = 0;
		for (uint32_t i = 0; i < fCnt; i++)
		{
			m_credits[i] = 0;
			m_nextAvail[i] = Time(0);
			m_findex_udpport_map[i] = 0;
			m_findex_qindex_map[i] = 0;
			m_waitingAck[i] = false;
			for (uint32_t j = 0; j < maxHop; j++)
			{
				m_txBytes[i][j] = m_bc;				//we don't need this at the beginning, so it doesn't matter what value it has
				m_rpWhile[i][j] = m_rpgTimeReset;	//we don't need this at the beginning, so it doesn't matter what value it has
				m_rpByteStage[i][j] = 0;
				m_rpTimeStage[i][j] = 0;
				m_alpha[i][j] = 0.5;
				m_rpStage[i][j] = 0; //not in any qcn stage
			}
		}
		for (uint32_t i = 0; i < pCnt; i++)
		{
			m_ECNState[i] = 0;
			m_ECNIngressCount[i] = 0;
			m_ECNEgressCount[i] = 0;
		}
	}

	QbbNetDevice::~QbbNetDevice()
	{
		NS_LOG_FUNCTION(this);
	}

	void
		QbbNetDevice::DoDispose()
	{
		NS_LOG_FUNCTION(this);
		// Cancel all the Qbb events
		for (uint32_t i = 0; i < qCnt; i++)
		{
			Simulator::Cancel(m_resumeEvt[i]);
		}

		for (uint32_t i = 0; i < fCnt; i++)
		{
			Simulator::Cancel(m_rateIncrease[i]);
		}

		for (uint32_t i = 0; i < pCnt; i++)
			for (uint32_t j = 0; j < qCnt; j++)
				Simulator::Cancel(m_recheckEvt[i][j]);
		PointToPointNetDevice::DoDispose();
	}

	void
		QbbNetDevice::TransmitComplete(void)
	{
		NS_LOG_FUNCTION(this);
		NS_ASSERT_MSG(m_txMachineState == BUSY, "Must be BUSY if transmitting");
		m_txMachineState = READY;
		NS_ASSERT_MSG(m_currentPkt != 0, "QbbNetDevice::TransmitComplete(): m_currentPkt zero");
		m_phyTxEndTrace(m_currentPkt);
		m_currentPkt = 0;
		DequeueAndTransmit();  //��������
	}

	void
		QbbNetDevice::DequeueAndTransmit(void)
	{
		//	std::cout << "DequeueAndTransmit" << std::endl;
		NS_LOG_FUNCTION(this);
		if (m_txMachineState == BUSY) return;	// Quit if channel busy
		Ptr<Packet> p;
		if (m_node->GetNodeType() == 0 && m_qcnEnabled) //���� QCN enable NIC   QCN����ʱ�Ż����QCN�㷨�����ʱ����������
		{
			p = m_queue->DequeueQCN(m_paused, m_nextAvail, m_findex_qindex_map);    //m_queues[0]������ȼ������ж�m_queues[0]��û��Ҫ���͵ģ���û���������ѯ
		}
		else if (m_node->GetNodeType() == 0 && m_timelyEnable) { //m_queues[0]������ȼ������ж�m_queues[0]��û��Ҫ���͵ģ���û����round-robin
			p = m_queue->DequeueTimely(m_paused, m_nextAvail, m_findex_qindex_map);
			//	std::cout << "DequeueTimely" << std::endl;
		}
		else if (m_node->GetNodeType() == 0) //QCN disable NIC
		{
			p = m_queue->DequeueNIC(m_paused);   //��ѯ
		}
		else   //switch, doesn't care about qcn, just send
		{
			//p = m_queue->Dequeue(m_paused);		//this is strict priority 

			p = m_queue->DequeueRR(m_paused);		//7��������ȼ������ϸ����ȼ�����û�ҵ���round-robin 
		}

		if (p != 0)
		{
			m_snifferTrace(p);
			m_promiscSnifferTrace(p);
			Ipv4Header h;
			Ptr<Packet> packet = p->Copy();
			uint16_t protocol = 0;
			ProcessHeader(packet, protocol);  //ȥ��PPP�ײ�
			packet->RemoveHeader(h);
			FlowIdTag t;
			if (m_node->GetNodeType() == 0) //I am a NIC, do QCN
			{								//QCN���������ı��������ʣ�����ͨ���ı�����´η��͵�ʱ�������ƿ�����������
				uint32_t fIndex = m_queue->GetLastQueue();   //ȡ�øոճ����е�index
				if (m_rate[fIndex] == 0)			//rate initialization	
				{
					if (m_timelyEnable)
						m_rate[fIndex] = initRate;	 //m_bps: ����Ĭ�ϵķ�������, ÿ�����ĳ�ʼ����
					else
						m_rate[fIndex] = m_bps;
					std::cout << "init rate: " << m_rate[fIndex].GetBitRate() << std::endl;
					for (uint32_t j = 0; j < maxHop; j++)
					{
						m_rateAll[fIndex][j] = m_bps;
						m_targetRate[fIndex][j] = m_bps; //< Target rate
					}
				}
				if (m_qcnEnabled) {   //qcn
					double creditsDue = std::max(0.0, m_bps / m_rate[fIndex] * (p->GetSize() - m_credits[fIndex]));
					Time nextSend = m_tInterframeGap + Seconds(m_bps.CalculateTxTime(creditsDue));
					m_nextAvail[fIndex] = Simulator::Now() + nextSend;

					for (uint32_t i = 0; i < m_queue->m_fcount; i++)	//distribute credits
					{
						if (m_nextAvail[i].GetTimeStep() <= Simulator::Now().GetTimeStep())
							m_credits[i] += m_rate[i] / m_bps*creditsDue;
					}
					m_credits[fIndex] = 0;	//reset credits

					for (uint32_t i = 0; i < 1; i++)
					{
						if (m_rpStage[fIndex][i] > 0) {	//1: fr; 2: ai; 3: hi   //��ʼ��Ϊ0��ֻ�е�enableQCN���յ���һ��cnp֮��m_rpStage�Ÿ�ֵ����0
							m_txBytes[fIndex][i] -= p->GetSize();
						}
						else
							m_txBytes[fIndex][i] = m_bc;  //< Tx byte counter
						if (m_txBytes[fIndex][i] <= 0)     //�������ֽڼ�����ָ����С�����ݣ�������Ӧ�Ľ׶θ��·�������
						{
							if (m_rpStage[fIndex][i] == 1)
							{
								rpr_fast_byte(fIndex, i);
							}
							else if (m_rpStage[fIndex][i] == 2)
							{
								rpr_active_byte(fIndex, i);
							}
							else if (m_rpStage[fIndex][i] == 3)
							{
								rpr_hyper_byte(fIndex, i);
							}
						}
					}
				}
				else if (m_timelyEnable) {   //timely
					segment_sent[fIndex] ++;
					if (segment_sent[fIndex] >= rtt_num) {
						segment_sent[fIndex] = 0;  //������0
						double creditsDue = std::max(0.0, m_bps / m_rate[fIndex] * (rtt_num * 1024 - m_credits[fIndex]));
						Time nextSend = m_tInterframeGap + Seconds(m_bps.CalculateTxTime(creditsDue));  //m_tInterframeGapĬ��=0

						m_nextAvail[fIndex] = Simulator::Now() + nextSend;

						for (uint32_t i = 0; i < m_queue->m_fcount; i++)	//distribute credits
						{
							if (m_nextAvail[i].GetTimeStep() <= Simulator::Now().GetTimeStep())
								m_credits[i] += m_rate[i] / m_bps*creditsDue;
						}
						m_credits[fIndex] = 0;	//reset credits
					}
				}

				if (h.GetProtocol() == 17 && m_waitAck) //if it's udp, check wait_for_ack
				{
					UdpHeader udph;
					packet->RemoveHeader(udph);
					SeqTsHeader sth;
					packet->RemoveHeader(sth);
					if (sth.GetSeq() >= m_milestone_tx[fIndex] - 1)
					{
						m_nextAvail[fIndex] = Simulator::Now() + Seconds(32767); //stop sending this flow until acked
						if (!m_waitingAck[fIndex])
						{
							//std::cout << "Waiting the ACK of the message of flow " << fIndex << " " << sth.GetSeq() << ".\n";
							//fflush(stdout);
							m_retransmit[fIndex] = Simulator::Schedule(MicroSeconds(m_waitAckTimer), &QbbNetDevice::Retransmit, this, fIndex);
							m_waitingAck[fIndex] = true;
						}
					}
				}

				p->RemovePacketTag(t);
				bool result = TransmitStart(p);

				if (result) {
					sent[fIndex] += p->GetSize();   //���������͵�������
				}
			}
			else //I am a switch, do ECN if this is not a pause
			{
				if (m_queue->GetLastQueue() == qCnt - 1)//this is a pause or cnp, send it immediately!
				{
					TransmitStart(p);
				}
				else
				{
					//switch ECN
					p->PeekPacketTag(t);
					uint32_t inDev = t.GetFlowId();  //���ո����ݰ��Ľ�����������
													 //���ڽ�����ֻ��8�����У��ȼ���ͬ����ͬһ���У�����˿ںͳ��˿ڵ�qIndex��ͬ
					m_node->m_broadcom->RemoveFromIngressAdmission(inDev, m_queue->GetLastQueue(), p->GetSize());
					m_node->m_broadcom->RemoveFromEgressAdmission(m_ifIndex, m_queue->GetLastQueue(), p->GetSize());
					if (m_qcnEnabled)
					{
						PppHeader ppp;
						p->RemoveHeader(ppp);
						p->RemoveHeader(h);
						bool egressCongested = ShouldSendCN(inDev, m_ifIndex, m_queue->GetLastQueue());
						if (egressCongested)
						{
							h.SetEcn((Ipv4Header::EcnType)0x03);
						}
						p->AddHeader(h);
						p->AddHeader(ppp);
					}
					p->RemovePacketTag(t);
					TransmitStart(p);
				}
			}
			return;
		}
		else //No queue can deliver any packet
		{
			//	std::cout << "û��Ҫ���͵�����" << std::endl;
			NS_LOG_INFO("PAUSE prohibits send at node " << m_node->GetId());
			if (m_node->GetNodeType() == 0 && (m_qcnEnabled || m_timelyEnable)) //nothing to send, possibly due to qcn flow control, if so reschedule sending
			{
				Time t = Simulator::GetMaximumSimulationTime();
				for (uint32_t i = 0; i < m_queue->m_fcount; i++)
				{
					if (m_queue->GetNBytes(i) == 0)
						continue;
					t = Min(m_nextAvail[i], t);
				}
				if (m_nextSend.IsExpired() &&   //�¼�����
					t < Simulator::GetMaximumSimulationTime() &&
					t.GetTimeStep() > Simulator::Now().GetTimeStep())
				{
					NS_LOG_LOGIC("Next DequeueAndTransmit at " << t << " or " << (t - Simulator::Now()) << " later");
					NS_ASSERT(t > Simulator::Now());
					m_nextSend = Simulator::Schedule(t - Simulator::Now(), &QbbNetDevice::DequeueAndTransmit, this);
				}
			}
		}
		return;
	}

	void
		QbbNetDevice::Retransmit(uint32_t findex)
	{
		std::cout << "Resending the message of flow " << findex << ".\n";
		fflush(stdout);
		m_queue->RecoverQueue(m_sendingBuffer[findex], findex);
		//��m_queue->m_queue[i]��գ�Ȼ��m_sendingBuffer�����ݼ���m_queue->m_queue[i]

		m_nextAvail[findex] = Simulator::Now();
		m_waitingAck[findex] = false;
		DequeueAndTransmit();
	}


	void
		QbbNetDevice::Resume(unsigned qIndex)
	{
		NS_LOG_FUNCTION(this << qIndex);
		NS_ASSERT_MSG(m_paused[qIndex], "Must be PAUSEd");
		m_paused[qIndex] = false;
		NS_LOG_INFO("Node " << m_node->GetId() << " dev " << m_ifIndex << " queue " << qIndex <<
			" resumed at " << Simulator::Now().GetSeconds());
		DequeueAndTransmit();
	}

	void
		QbbNetDevice::PauseFinish(unsigned qIndex)
	{
		Resume(qIndex);
	}


	void
		QbbNetDevice::Receive(Ptr<Packet> packet)
	{
		NS_LOG_FUNCTION(this << packet);

		if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
		{
			// 
			// If we have an error model and it indicates that it is time to lose a
			// corrupted packet, don't forward this packet up, let it go.
			//
			m_phyRxDropTrace(packet);
			return;
		}
		uint16_t protocol = 0;
		Ptr<Packet> p = packet->Copy();
		ProcessHeader(p, protocol);   //ȥ��PPP�ײ�

		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);

		if ((ipv4h.GetProtocol() != 0xFF && ipv4h.GetProtocol() != 0xFD && ipv4h.GetProtocol() != 0xFC) || m_node->GetNodeType() > 0)
		{ //This is not QCN feedback, not NACK, not ACK or I am a switch so I don't care
			if (ipv4h.GetProtocol() != 0xFE) //not PFC
			{
				packet->AddPacketTag(FlowIdTag(m_ifIndex));   //m_ifIndex�������������ţ����յ������ݰ�����������
				if (m_node->GetNodeType() == 0) //NIC
				{
					if (ipv4h.GetProtocol() == 17)	//look at udp only  �������յ�һ�����ݰ�
					{
						//std::cout << "�����յ����ݰ�" << std::endl;
						uint16_t ecnbits = ipv4h.GetEcn();
						uint64_t now = Simulator::Now().GetTimeStep();
						UdpHeader udph;
						p->RemoveHeader(udph);
						SeqTsHeader sth;
						p->PeekHeader(sth);
						uint64_t ts;   //�յ������ݰ�����timeStamp������timeStamp���ڷ��ص�ACK����
						ts = sth.GetTs().GetTimeStep();
						p->AddHeader(udph);

						bool found = false;
						uint32_t i, key = 0;

						for (i = 0; i < m_ecn_source->size(); ++i)
						{
							if ((*m_ecn_source)[i].source == ipv4h.GetSource() && (*m_ecn_source)[i].qIndex == GetPriority(p->Copy()) && (*m_ecn_source)[i].port == udph.GetSourcePort())
							{
								found = true;
								if (ecnbits != 0 && Simulator::Now().GetMicroSeconds() > m_qcn_np_sampling) //m_qcn_np_samplingĬ��=0
								{
									(*m_ecn_source)[i].ecnbits |= ecnbits;
									(*m_ecn_source)[i].qfb++;
								}
								(*m_ecn_source)[i].total++;
								key = i;
								break;
							}
						}
						if (!found)
						{
							ECNAccount tmp;
							tmp.qIndex = GetPriority(p->Copy());
							tmp.source = ipv4h.GetSource();
							if (ecnbits != 0 && Simulator::Now().GetMicroSeconds() > m_qcn_np_sampling && tmp.qIndex != 1) //dctcp
							{
								tmp.ecnbits |= ecnbits;
								tmp.qfb = 1;
							}
							else
							{
								tmp.ecnbits = 0;
								tmp.qfb = 0;
							}
							tmp.total = 1;
							tmp.port = udph.GetSourcePort();
							ReceiverNextExpectedSeq[m_ecn_source->size()] = 0;
							m_nackTimer[m_ecn_source->size()] = Time(0);
							m_milestone_rx[m_ecn_source->size()] = m_ack_interval;
							m_lastNACK[m_ecn_source->size()] = -1;
							key = m_ecn_source->size();
							received[key] = 0;
							m_ecn_source->push_back(tmp);
							CheckandSendQCN(tmp.source, tmp.qIndex, tmp.port); //ÿ���µ��������cnp_interval���øú����������ж��Ƿ���cnp
																			   //�������ȼ�����pause
						}

						received[key] ++;
						m_received += packet->GetSize();

						rtt_half[key] += now - ts;
						//std::cout << "half: " << now - ts << std::endl;
						int x = ReceiverCheckSeq(sth.GetSeq(), key);

						if (x == 2) //generate NACK
						{
							//std::cout << "generate NACK" << std::endl;
							Ptr<Packet> newp = Create<Packet>(0);
							SeqTsHeader sth;       //ʵ��timely 
							if (m_qcnEnabled || m_timelyEnable)    //ack��SeqTsHeader��pgûʲô�ô����õ�����ack��qbbHeader��pg
								sth.SetPG(0);   //������ȼ�
							else
								sth.SetPG(qCnt - 1);
							if (m_timelyEnable) {
								if (received[key] >= rtt_num) {
									received[key] = 0;
									sth.SetTs(now - rtt_half[key] / rtt_num);
									rtt_half[key] = 0;
								}
								else
									sth.SetTs(0);
							}
							else
								sth.SetTs(ts);
							newp->AddHeader(sth);

							qbbHeader seqh;
							seqh.SetSeq(ReceiverNextExpectedSeq[key]);
							seqh.SetPG(GetPriority(p->Copy()));
							seqh.SetPort(udph.GetSourcePort());
							newp->AddHeader(seqh);
							Ipv4Header head;	// Prepare IPv4 header
							head.SetDestination(ipv4h.GetSource());
							Ipv4Address myAddr = m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal();
							head.SetSource(myAddr);
							head.SetProtocol(0xFD); //nack=0xFD
							head.SetTtl(64);
							head.SetPayloadSize(newp->GetSize());
							head.SetIdentification(UniformVariable(0, 65536).GetValue());
							newp->AddHeader(head);
							uint32_t protocolNumber = 2048;
							AddHeader(newp, protocolNumber);	// Attach PPP header
							if (m_qcnEnabled)
								m_queue->Enqueue(newp, 0);
							else if (m_timelyEnable)
								m_queue->Enqueue(newp, 0);
							else
								m_queue->Enqueue(newp, qCnt - 1);
							DequeueAndTransmit();
						}
						//	else if (x == 1) //generate ACK
						else if (x == 1 || x == 3)   //����ACK�� x == 3 ��ʾ Duplicate
						{
							if (x == 3)
								std::cout << "qbb-net-device Duplicate" << std::endl;
							Ptr<Packet> newp = Create<Packet>(0);
							SeqTsHeader sth;       //ʵ��timely 
							if (m_qcnEnabled || m_timelyEnable)    //ack��SeqTsHeader��pgûʲô�ô����õ�����ack��qbbHeader��pg
								sth.SetPG(0);   //������ȼ�
							else
								sth.SetPG(qCnt - 1);

							if (m_timelyEnable) {
								if (received[key] >= rtt_num) {   
									received[key] = 0;
									sth.SetTs(now - rtt_half[key] / rtt_num);
									rtt_half[key] = 0;
								}
								else
									sth.SetTs(0);
							}
							else
								sth.SetTs(ts);
							newp->AddHeader(sth);

							qbbHeader qbb;
							qbb.SetSeq(ReceiverNextExpectedSeq[key]);
							qbb.SetPG(GetPriority(p->Copy()));  //���ص�ACK�����ȼ���Ӧ�����յ����ݰ������ȼ�һ��
							qbb.SetPort(udph.GetSourcePort());
							newp->AddHeader(qbb);
							Ipv4Header head;	// Prepare IPv4 header
							head.SetDestination(ipv4h.GetSource());
							Ipv4Address myAddr = m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal();
							head.SetSource(myAddr);
							head.SetProtocol(0xFC); //ack=0xFC
							head.SetTtl(64);
							head.SetPayloadSize(newp->GetSize());
							head.SetIdentification(UniformVariable(0, 65536).GetValue());
							newp->AddHeader(head);
							uint32_t protocolNumber = 2048;
							AddHeader(newp, protocolNumber);	// Attach PPP header
							if (m_qcnEnabled)
								m_queue->Enqueue(newp, 0);
							else if (m_timelyEnable)
								m_queue->Enqueue(newp, 0);
							else
								m_queue->Enqueue(newp, qCnt - 1);
							DequeueAndTransmit();
						}
					}
				}
				PointToPointReceive(packet);
			}
			else // If this is a Pause, stop the corresponding queue
			{
				//	std::cout << "�յ�PFC" << std::endl;
				//	return;

				if (!m_qbbEnabled) return;
				PauseHeader pauseh;
				p->RemoveHeader(pauseh);
				unsigned qIndex = pauseh.GetQIndex();
				m_paused[qIndex] = true;  //��ͣ���ȼ�ΪqIndex����
				if (pauseh.GetTime() > 0)
				{
					Simulator::Cancel(m_resumeEvt[qIndex]);
					m_resumeEvt[qIndex] = Simulator::Schedule(MicroSeconds(pauseh.GetTime()), &QbbNetDevice::PauseFinish, this, qIndex);
				}
				else
				{
					Simulator::Cancel(m_resumeEvt[qIndex]);
					PauseFinish(qIndex);
				}
			}
		}
		else if (ipv4h.GetProtocol() == 0xFF)
		{
			// QCN on NIC
			// This is a Congestion signal
			// Then, extract data from the congestion packet.
			// We assume, without verify, the packet is destinated to me
			CnHeader cnHead;
			p->RemoveHeader(cnHead);
			uint32_t qIndex = cnHead.GetQindex();
			if (qIndex == 1)		//DCTCP
			{
				std::cout << "TCP--ignore\n";  //δʵ��DCTCP
				return;
			}
			uint32_t udpport = cnHead.GetFlow();
			uint16_t ecnbits = cnHead.GetECNBits();
			uint16_t qfb = cnHead.GetQfb();
			uint16_t total = cnHead.GetTotal();

			uint32_t i;
			for (i = 1; i < m_queue->m_fcount; i++)
			{
				if (m_findex_udpport_map[i] == udpport && m_findex_qindex_map[i] == qIndex)
					break;
			}
			if (i == m_queue->m_fcount)
				std::cout << "ERROR: QCN NIC cannot find the flow\n";

			if (qfb == 0)
			{
				std::cout << "ERROR: Unuseful QCN\n";
				return;	// Unuseful CN
			}
			if (m_rate[i] == 0)			//lazy initialization	
			{
				m_rate[i] = m_bps;
				for (uint32_t j = 0; j < maxHop; j++)
				{
					m_rateAll[i][j] = m_bps;
					m_targetRate[i][j] = m_bps;	//targetrate remembers the last rate
				}
			}
			if (ecnbits == 0x03)
			{
				rpr_cnm_received(i, 0, qfb*1.0 / (total + 1));
			}
			m_rate[i] = m_bps;
			for (uint32_t j = 0; j < maxHop; j++)
				m_rate[i] = std::min(m_rate[i], m_rateAll[i][j]);
			PointToPointReceive(packet);
		}

		else if (ipv4h.GetProtocol() == 0xFD)//NACK on NIC
		{
			qbbHeader qbbh;
			p->Copy()->RemoveHeader(qbbh);   //��ʱ��p�Ѿ�ȥ��ppp,ip�ײ�

			int qIndex = qbbh.GetPG();
			int seq = qbbh.GetSeq();
			int port = qbbh.GetPort();
			int i;
			for (i = 1; i < m_queue->m_fcount; i++)
			{
				if (m_findex_udpport_map[i] == port && m_findex_qindex_map[i] == qIndex)
				{
					break;
				}
			}
			if (i == m_queue->m_fcount)
			{
				std::cout << "ERROR: NACK NIC cannot find the flow\n";
			}

			uint32_t key = i;
			SeqTsHeader sth;
			packet->PeekHeader(sth); //�˴�Ҫ����PeekHeader����Copy()������ֱ�ӵ���RemoveHeader()�����ᱨ��������
			int64_t ts = sth.GetTs().GetTimeStep();
		/*	if (!m_timelyEnable) {
				new_rtt = Simulator::Now().GetTimeStep() - ts;
				uint64_t noise = UniformVariable(0, rtt_noise).GetValue();
				new_rtt += noise;
				m_rtt = new_rtt;
				rtt[key] = new_rtt;
			}
			else if (ts > 0 && m_ack_interval > 0) {
				new_rtt = Simulator::Now().GetTimeStep() - ts;
				uint64_t noise = UniformVariable(0, rtt_noise).GetValue();
				new_rtt += noise;
				m_rtt = new_rtt;
				rtt[key] = new_rtt;
				if (minRTT[key] != 0) {
					if (minRTT[key] > new_rtt)
						minRTT[key] = new_rtt;
				}
				else
					minRTT[key] = new_rtt;

				if (prev_rtt[key] == 0)
					prev_rtt[key] = new_rtt;
				else {
					double new_rtt_diff = new_rtt - prev_rtt[key];  //�˴��������޷�������new_rtt_diff
					prev_rtt[key] = new_rtt;
					rtt_diff[key] = (1 - weight_rtt_diff) * rtt_diff[key] + weight_rtt_diff * new_rtt_diff; //�˴��������޷�������rtt_diff�����������long long<-double
					double normalized_gradient = (double)rtt_diff[key] / minRTT[key];   //minRTTֵ����ȶ
					if (new_rtt < rtt_low) {
						m_rate[key] = m_rate[key] + step;
						N = 0;   //����Ҫ��Ҫȥ�����߸ĳ�N++
								 //std::cout << "ֱ�Ӽ���" << std::endl;
					}
					else if (new_rtt > rtt_hight) {
						double a = rtt_hight;
						double b = new_rtt;
						double tmp = 1 - weight_bida * (1 - a / b);
						m_rate[key] = m_rate[key] * tmp;
						N = 0;
					}

*/
					//Patched Timely
					/*	else {
					double weight;
					if (normalized_gradient <= -0.25)
					weight = 0;
					else if (normalized_gradient >= 0.25)
					weight = 1;
					else
					weight = 2 * normalized_gradient + 0.5;
					double error = (double)(new_rtt - rtt_ref) / (double)rtt_ref;
					m_rate[key] = step * (1 - weight) + m_rate[key] * (1 - weight_bida * weight * error);
					//	std::cout << rtt_ref << std::endl;
					}
					*/	//Original Timely

/*
					if (normalized_gradient <= 0) {
						if (N >= 5) {
							m_rate[key] = m_rate[key] + 5 * step;
						}
						else {
							m_rate[key] = m_rate[key] + step;
						}
						N++;
					}
					else {
						double tmp = 1 - weight_bida * normalized_gradient;
						if (tmp <= 0)
							m_rate[key] = m_minRate;
						else
							m_rate[key] = m_rate[key] * tmp;
						N = 0;
					}
					if (m_rate[key] > m_bps) {  //���ʲ��ܴ�����������
						m_rate[key] = m_bps;
					}
					else if (m_rate[key] < m_minRate) {
						m_rate[key] = m_minRate;   //100Mbps   �������
					}
				}
				rate = m_rate[key].GetBitRate();
			}
*/
			uint32_t buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
			if (!m_backto0)
			{
				if (buffer_seq > seq)
				{
					std::cout << "ERROR: Sendingbuffer miss!\n";
				}
				while (seq > buffer_seq)
				{
					m_sendingBuffer[i]->Dequeue();
					buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
				}
			}
			else
			{
				uint32_t goback_seq = seq / m_chunk*m_chunk;
				if (buffer_seq > goback_seq)
				{
					std::cout << "ERROR: Sendingbuffer miss!\n";
				}
				while (goback_seq > buffer_seq)
				{
					m_sendingBuffer[i]->Dequeue();
					buffer_seq = GetSeq(m_sendingBuffer[i]->Peek()->Copy());
				}

			}

			m_queue->RecoverQueue(m_sendingBuffer[i], i);

			if (m_waitAck && m_waitingAck[i])
			{
				m_nextAvail[i] = Simulator::Now();
				Simulator::Cancel(m_retransmit[i]);
				m_waitingAck[i] = false;
				DequeueAndTransmit();
			}
			PointToPointReceive(packet);
		}
		else if (ipv4h.GetProtocol() == 0xFC)//ACK on NIC
		{
			qbbHeader qbbh;
			SeqTsHeader sth;
			//��ʱp�Ѿ�ȥ����PPP��IP�ײ�
			Ptr<Packet> packet = p->Copy();
			packet->RemoveHeader(qbbh);
			uint16_t port = qbbh.GetPort();
			uint16_t qIndex = qbbh.GetPG();

			packet->PeekHeader(sth); //�˴�Ҫ����PeekHeader����Copy()������ֱ�ӵ���RemoveHeader()�����ᱨ��������
			int64_t ts = sth.GetTs().GetTimeStep();
			uint32_t i;
			for (i = 1; i < m_queue->m_fcount; i++) {   //�ҵ���Ӧ����
				if (m_findex_qindex_map[i] == qIndex && m_findex_udpport_map[i] == port)
					break;
			}
			if (i >= m_queue->m_fcount) {
				std::cout << "ERROR in qbb-net-device.cc 'ACK on NIC': �Ҳ�����Ӧ������" << std::endl;
			}
			uint32_t key = i;
			if (!m_timelyEnable) {
				new_rtt = Simulator::Now().GetTimeStep() - ts;
				uint64_t noise = UniformVariable(0, rtt_noise).GetValue();
				new_rtt += noise;
				m_rtt = new_rtt;
				rtt[key] = new_rtt;
			//	std::cout << "DCQCN: " << new_rtt << std::endl;
			}
			else if (ts > 0 && m_ack_interval > 0) {
				ack_cnt++;
				new_rtt = Simulator::Now().GetTimeStep() - ts;

				std::cout << "qbb-net-device: RTT " << new_rtt << std::endl;  //add

				uint64_t noise = UniformVariable(0, rtt_noise).GetValue();
				new_rtt += noise;

				m_rtt = new_rtt;
				rtt[key] = new_rtt;
			/*	if (minRTT[key] != 0) {   
					if (minRTT[key] > new_rtt)
						minRTT[key] = new_rtt;
				}
				else
					minRTT[key] = new_rtt;
			*/
				if (prev_rtt[key] == 0)
					prev_rtt[key] = new_rtt;
				else {
					double new_rtt_diff = new_rtt - prev_rtt[key];  //�˴��������޷�������new_rtt_diff
					prev_rtt[key] = new_rtt;
					rtt_diff[key] = (1 - weight_rtt_diff) * rtt_diff[key] + weight_rtt_diff * new_rtt_diff; //�˴��������޷�������rtt_diff�����������long long<-double
					double normalized_gradient = (double)rtt_diff[key] / m_min_rtt;   //minRTTֵ����ȶ
					if (new_rtt < rtt_low) {
						m_rate[key] = m_rate[key] + step;
						N = 0;   //����Ҫ��Ҫȥ�����߸ĳ�N++
								 //std::cout << "ֱ�Ӽ���" << std::endl;
					//	std::cout << "С��rtt_low" << std::endl;
					}
					else if (new_rtt > rtt_hight) {
						double a = rtt_hight;
						double b = new_rtt;
						double tmp = 1 - weight_bida * (1 - a / b);
					//	std::cout << "����rtt_high:" << tmp << std::endl;
						m_rate[key] = m_rate[key] * tmp;
						N = 0;
					//	std::cout << "����rtt_high" << std::endl;
					}
					//Patched Timely
					/*	else {
					double weight;
					if (normalized_gradient <= -0.25)
					weight = 0;
					else if (normalized_gradient >= 0.25)
					weight = 1;
					else
					weight = 2 * normalized_gradient + 0.5;
					double error = (double)(new_rtt - rtt_ref) / (double)rtt_ref;
					m_rate[key] = step * (1 - weight) + m_rate[key] * (1 - weight_bida * weight * error);
					//	std::cout << rtt_ref << std::endl;
					}*/
					//Original Timely
					else if (normalized_gradient <= 0) {   // ԭ���� ��<=��
					 //	std::cout << "����" << std::endl;
						if (N >= 5) {
							m_rate[key] = m_rate[key] + 5 * step;
						}
						else {
							m_rate[key] = m_rate[key] + step;
						}
						N++;
					}
					else {
						
						double tmp = 1 - weight_bida * normalized_gradient;
				//		std::cout << "����  tmp:" << tmp << std::endl;
						if (tmp <= 0)
							m_rate[key] = m_minRate;
						else 
							m_rate[key] = m_rate[key] * tmp;
						N = 0;
					}
					if (m_rate[key] > m_bps) {  //���ʲ��ܴ�����������
						m_rate[key] = m_bps;
					}
					else if (m_rate[key] < m_minRate) {
						m_rate[key] = m_minRate;   //100Mbps   �������
					}
				}
				rate = m_rate[key].GetBitRate();
				//	return; // ts !=0 ʱ�� ACKֻ����������RTT
			}

			int seq = qbbh.GetSeq();
			uint32_t buffer_seq = GetSeq(m_sendingBuffer[key]->Peek()->Copy());  //����m_sendingBuffer�еĵ�һ��packet

			if (m_ack_interval == 0)    //L2��֧�ֲ�����ACK����
			{
				std::cout << "ERROR: shouldn't receive ack\n";
			}
			else
			{
				if (!m_backto0)
				{
					while (seq > buffer_seq)       //��ACKȷ�ϵ����кŴ���m_sendingBuffer����packet�����к�
					{
						m_sendingBuffer[key]->Dequeue();   //���׵�packet�յ�ACK������ɾ��
						if (m_sendingBuffer[key]->IsEmpty())
						{
							break;
						}
						buffer_seq = GetSeq(m_sendingBuffer[key]->Peek()->Copy());
					}
				}
				else
				{
					uint32_t goback_seq = seq / m_chunk * m_chunk;   //�м������̿��ܲ���������
					while (goback_seq > buffer_seq)
					{
						m_sendingBuffer[key]->Dequeue();
						if (m_sendingBuffer[key]->IsEmpty())
						{
							break;
						}
						buffer_seq = GetSeq(m_sendingBuffer[key]->Peek()->Copy());
					}
				}
			}

			if (m_waitAck && seq >= m_milestone_tx[key])
			{
				//Got ACK, resume sending
				m_nextAvail[key] = Simulator::Now();
				Simulator::Cancel(m_retransmit[key]);
				m_waitingAck[key] = false;
				m_milestone_tx[key] += m_chunk;
				DequeueAndTransmit();

			}

			PointToPointReceive(packet);

			//	DequeueAndTransmit(); //���ӵ� 
		}
	}

	void
		QbbNetDevice::PointToPointReceive(Ptr<Packet> packet)
	{
		NS_LOG_FUNCTION(this << packet);
		uint16_t protocol = 0;

		if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt(packet))
		{
			// 
			// If we have an error model and it indicates that it is time to lose a
			// corrupted packet, don't forward this packet up, let it go.
			//
			m_phyRxDropTrace(packet);
		}
		else
		{
			// 
			// Hit the trace hooks.  All of these hooks are in the same place in this 
			// device becuase it is so simple, but this is not usually the case in 
			// more complicated devices.
			//
			m_snifferTrace(packet);
			m_promiscSnifferTrace(packet);
			m_phyRxEndTrace(packet);
			//
			// Strip off the point-to-point protocol header and forward this packet
			// up the protocol stack.  Since this is a simple point-to-point link,
			// there is no difference in what the promisc callback sees and what the
			// normal receive callback sees.
			//
			ProcessHeader(packet, protocol);

			if (!m_promiscCallback.IsNull())
			{
				m_macPromiscRxTrace(packet);
				m_promiscCallback(this, packet, protocol, GetRemote(), GetAddress(), NetDevice::PACKET_HOST);
			}

			m_macRxTrace(packet);
			m_rxCallback(this, packet, protocol, GetRemote());
		}
	}

	uint32_t
		QbbNetDevice::GetPriority(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		UdpHeader udph;
		p->RemoveHeader(udph);
		SeqTsHeader seqh;
		p->RemoveHeader(seqh);
		return seqh.GetPG();
	}

	uint32_t
		QbbNetDevice::GetSeq(Ptr<Packet> p) //Pay attention this function modifies the packet!!! Copy the packet before passing in.
	{
		uint16_t protocol;
		ProcessHeader(p, protocol);
		Ipv4Header ipv4h;
		p->RemoveHeader(ipv4h);
		UdpHeader udph;
		p->RemoveHeader(udph);
		SeqTsHeader seqh;
		p->RemoveHeader(seqh);
		return seqh.GetSeq();
	}

	bool
		QbbNetDevice::Send(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
	{
		NS_LOG_FUNCTION(this << packet << dest << protocolNumber);
		NS_LOG_LOGIC("UID is " << packet->GetUid());
		if (IsLinkUp() == false) {        //�ж���·�Ƿ���up
			m_macTxDropTrace(packet);
			return false;
		}

		Ipv4Header h;
		packet->PeekHeader(h);   //����packet��IP�ײ���������IP�ײ�����
		unsigned qIndex;
		uint8_t prot = h.GetProtocol();
		if (prot == 0xFF || prot == 0xFE || prot == 0xFD || prot == 0xFC)  //QCN or PFC or NACK or ACK, go highest priority
		{
			qIndex = qCnt - 1;
		}
		else    //udp-echo-clientʹ�õ�Э��ΪUDP��17��
		{
			Ptr<Packet> p = packet->Copy();
			p->RemoveHeader(h);

			if (h.GetProtocol() == 17)     //�ϲ�Э��ΪUDPЭ��    
				qIndex = GetPriority(p);   // ��ʱp�Ѿ�ȥ����IP�ײ�
			else
				qIndex = 1; //dctcp
		}

		Ptr<Packet> p = packet->Copy();
		AddHeader(packet, protocolNumber);  //����PPP�ײ�

		if (m_node->GetNodeType() == 0)     //����
		{
			if ((m_qcnEnabled || m_timelyEnable) && qIndex == qCnt - 1)
			{
				m_queue->Enqueue(packet, 0); //����ʱ�����ȿ��Ƕ���0
			}
			else
			{
				Ipv4Header ipv4h;
				p->RemoveHeader(ipv4h);
				UdpHeader udph;
				p->RemoveHeader(udph);
				uint32_t port = udph.GetSourcePort();
				uint32_t i;
				for (i = 1; i < fCnt; i++)
				{
					if (m_findex_udpport_map[i] == 0)
					{
						m_queue->m_fcount = i + 1;
						m_findex_udpport_map[i] = port;
						m_findex_qindex_map[i] = qIndex;
						m_sendingBuffer[i] = CreateObject<DropTailQueue>();
						segment_sent[i] = 0;
						if (m_waitAck)
						{
							m_milestone_tx[i] = m_chunk;
						}
						break;
					}
					if (m_findex_udpport_map[i] == port && m_findex_qindex_map[i] == qIndex)
						break;
				}
				if (m_sendingBuffer[i]->GetNPackets() == 8000)  //���ͻ������ʱ���������׵İ�
				{
					m_sendingBuffer[i]->Dequeue();
				}
				m_sendingBuffer[i]->Enqueue(packet->Copy());

				if (m_qcnEnabled || m_timelyEnable)
				{
					m_queue->Enqueue(packet, i);   //��֧��QCN,TIMELY���ƣ���m_queue�ж�����Ŀ���Ϊfcnt
				}
				else
				{
					m_queue->Enqueue(packet, qIndex);  //����m_queue�ж�����ĿΪ8   qIndex => i  aaa
				}
			}
			DequeueAndTransmit();
		}
		else //switch
		{
			//AddHeader(packet, protocolNumber);
			if (qIndex != qCnt - 1)			//not pause frame   ����QCN,NACK,ACK,PFC
			{
				FlowIdTag t;
				packet->PeekPacketTag(t);
				uint32_t inDev = t.GetFlowId();  //���ո����ݰ��Ľ�����������
				if (m_node->m_broadcom->CheckIngressAdmission(inDev, qIndex, packet->GetSize()) && m_node->m_broadcom->CheckEgressAdmission(m_ifIndex, qIndex, packet->GetSize()))			// Admission control
				{	//inDev�����������˿ڣ���id��m_ifIndex�����������˿ڣ���id
					//���ڽ�����ֻ��8�����У��ȼ���ͬ����ͬһ���У�����˿ںͳ��˿ڵ�qIndex��ͬ
					m_node->m_broadcom->UpdateIngressAdmission(inDev, qIndex, packet->GetSize());
					m_node->m_broadcom->UpdateEgressAdmission(m_ifIndex, qIndex, packet->GetSize());

					m_macTxTrace(packet);
					m_queue->Enqueue(packet, qIndex); // go into MMU and queues
				}
				DequeueAndTransmit();
				if (m_node->GetNodeType() == 1)
				{
					CheckQueueFull(inDev, qIndex); //check queue full  �����˿ڣ��ж��Ƿ���PFC
				}
			}
			else			//ֱ��ת��QCN,NACK,ACK,PFC
			{
				m_queue->Enqueue(packet, qIndex);
				DequeueAndTransmit();
			}

		}
		return true;
	}

	void
		QbbNetDevice::CheckQueueFull(uint32_t inDev, uint32_t qIndex)  //Function to generate PAUSE message if necessary
	{																	//ֻ�н������Żᷢ��PFC
		NS_LOG_FUNCTION(this);
		Ptr<Ipv4> m_ipv4 = m_node->GetObject<Ipv4>();
		bool pClasses[qCnt] = { 0 };
		m_node->m_broadcom->GetPauseClasses(inDev, qIndex, pClasses);
		Ptr<NetDevice> device = m_ipv4->GetNetDevice(inDev);
		for (uint32_t j = 0; j < qCnt; j++)
		{
			if (pClasses[j])			// Create the PAUSE packet
			{
				Ptr<Packet> p = Create<Packet>(0);
				PauseHeader pauseh(m_pausetime, m_queue->GetNBytes(j), j);
				p->AddHeader(pauseh);
				Ipv4Header ipv4h;  // Prepare IPv4 header
				ipv4h.SetProtocol(0xFE);
				ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
				ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
				ipv4h.SetPayloadSize(p->GetSize());
				ipv4h.SetTtl(1);
				ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
				p->AddHeader(ipv4h);
				device->Send(p, Mac48Address("ff:ff:ff:ff:ff:ff"), 0x0800);
				m_node->m_broadcom->m_pause_remote[inDev][qIndex] = true;
				Simulator::Cancel(m_recheckEvt[inDev][qIndex]);
				m_recheckEvt[inDev][qIndex] = Simulator::Schedule(MicroSeconds(m_pausetime / 2), &QbbNetDevice::CheckQueueFull, this, inDev, qIndex);
			}
		}

		//ON-OFF
		for (uint32_t j = 0; j < qCnt; j++)
		{
			if (!m_node->m_broadcom->m_pause_remote[inDev][qIndex])
				continue;
			if (m_node->m_broadcom->GetResumeClasses(inDev, qIndex))  //���б�С��֪ͨ���ν��PFC
			{
				Ptr<Packet> p = Create<Packet>(0);
				PauseHeader pauseh(0, m_queue->GetNBytes(j), j); //resume   paused time = 0
				p->AddHeader(pauseh);
				Ipv4Header ipv4h;  // Prepare IPv4 header
				ipv4h.SetProtocol(0xFE);
				ipv4h.SetSource(m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal());
				ipv4h.SetDestination(Ipv4Address("255.255.255.255"));
				ipv4h.SetPayloadSize(p->GetSize());
				ipv4h.SetTtl(1);
				ipv4h.SetIdentification(UniformVariable(0, 65536).GetValue());
				p->AddHeader(ipv4h);
				device->Send(p, Mac48Address("ff:ff:ff:ff:ff:ff"), 0x0800);
				m_node->m_broadcom->m_pause_remote[inDev][qIndex] = false;
				Simulator::Cancel(m_recheckEvt[inDev][qIndex]);
			}
		}
	}

	bool
		QbbNetDevice::IsLocal(const Ipv4Address& addr) const
	{
		Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4>();
		for (unsigned i = 0; i < ipv4->GetNInterfaces(); i++) {
			for (unsigned j = 0; j < ipv4->GetNAddresses(i); j++) {
				if (ipv4->GetAddress(i, j).GetLocal() == addr) {
					return true;
				};
			};
		};
		return false;
	}

	void
		QbbNetDevice::ConnectWithoutContext(const CallbackBase& callback)
	{
		NS_LOG_FUNCTION(this);
		m_sendCb.ConnectWithoutContext(callback);
	}

	void
		QbbNetDevice::DisconnectWithoutContext(const CallbackBase& callback)
	{
		NS_LOG_FUNCTION(this);
		m_sendCb.DisconnectWithoutContext(callback);
	}

	int32_t
		QbbNetDevice::PrintStatus(std::ostream& os)
	{
		os << "Size:";
		uint32_t sum = 0;
		for (unsigned i = 0; i < qCnt; ++i) {
			os << " " << (m_paused[i] ? "q" : "Q") << "[" << i << "]=" << m_queue->GetNBytes(i);
			sum += m_queue->GetNBytes(i);
		};
		os << " sum=" << sum << std::endl;
		return sum;
	};

	bool
		QbbNetDevice::Attach(Ptr<QbbChannel> ch)
	{
		NS_LOG_FUNCTION(this << &ch);
		m_channel = ch;
		m_channel->Attach(this);
		NotifyLinkUp();
		return true;
	}

	bool
		QbbNetDevice::TransmitStart(Ptr<Packet> p)
	{
		NS_LOG_FUNCTION(this << p);
		NS_LOG_LOGIC("UID is " << p->GetUid() << ")");
		//
		// This function is called to start the process of transmitting a packet.
		// We need to tell the channel that we've started wiggling the wire and
		// schedule an event that will be executed when the transmission is complete.
		//
		NS_ASSERT_MSG(m_txMachineState == READY, "Must be READY to transmit");
		m_txMachineState = BUSY;
		m_currentPkt = p;
		m_phyTxBeginTrace(m_currentPkt);
		Time txTime = Seconds(m_bps.CalculateTxTime(p->GetSize()));   //������������ʱ��
		Time txCompleteTime = txTime + m_tInterframeGap;				//�������ʱ��
		NS_LOG_LOGIC("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds() << "sec");
		Simulator::Schedule(txCompleteTime, &QbbNetDevice::TransmitComplete, this);   //����txCompleteTime��ʱ���������ͣ��˴�ģ�������ķ���ʱ��    

																					  //Ptr<Packet> pp = p->Copy(); 
		if (m_timelyEnable && m_node->GetNodeType() == 0) {
			PppHeader pppHeader;
			Ipv4Header ipHeader;

			p->RemoveHeader(pppHeader);
			p->RemoveHeader(ipHeader);
			if (ipHeader.GetProtocol() == 17) {  //ֻ�����ݰ��м���ts
				UdpHeader udpHeader;
				SeqTsHeader seqThHeader;
				p->RemoveHeader(udpHeader);
				p->RemoveHeader(seqThHeader);
				seqThHeader.SetTs(Simulator::Now().GetTimeStep());
				p->AddHeader(seqThHeader);
				p->AddHeader(udpHeader);
			}
			p->AddHeader(ipHeader);
			p->AddHeader(pppHeader);
		}

		//	std::cout << "׼������" << std::endl;
		bool result = m_channel->TransmitStart(p, this, txTime);
		if (result == false)
		{
			m_phyTxDropTrace(p);
		}
		else {
			m_sent += p->GetSize();  //�Ѿ����͵��ֽ���
									 //	std::cout << "������: " << m_sent << std::endl;
		}
		return result;
	}

	Address
		QbbNetDevice::GetRemote(void) const
	{
		NS_ASSERT(m_channel->GetNDevices() == 2);
		for (uint32_t i = 0; i < m_channel->GetNDevices(); ++i)
		{
			Ptr<NetDevice> tmp = m_channel->GetDevice(i);
			if (tmp != this)
			{
				return tmp->GetAddress();
			}
		}
		NS_ASSERT(false);
		return Address();  // quiet compiler.
	}

	//�������Ƿ���CE��־
	bool
		QbbNetDevice::ShouldSendCN(uint32_t indev, uint32_t ifindex, uint32_t qIndex)
	{
		return m_node->m_broadcom->ShouldSendCN(indev, ifindex, qIndex);
	}

	//���շ�����cnp��ÿm_qcn_interval΢�����Ƿ���cnp
	void
		QbbNetDevice::CheckandSendQCN(Ipv4Address source, uint32_t qIndex, uint32_t port) //port is udp port
	{
		if (m_node->GetNodeType() > 0)
			return;
		if (!m_qcnEnabled)
			return;
		for (uint32_t i = 0; i < m_ecn_source->size(); ++i)
		{
			ECNAccount info = (*m_ecn_source)[i];
			if (info.source == source && info.qIndex == qIndex && info.port == port)
			{
				if (info.ecnbits == 0x03)    //ӵ���ź�
				{
					Ptr<Packet> p = Create<Packet>(0);
					CnHeader cn(port, qIndex, info.ecnbits, info.qfb, info.total);	// Prepare CN header
					p->AddHeader(cn);
					Ipv4Header head;	// Prepare IPv4 header
					head.SetDestination(source);
					Ipv4Address myAddr = m_node->GetObject<Ipv4>()->GetAddress(m_ifIndex, 0).GetLocal();
					head.SetSource(myAddr);
					head.SetProtocol(0xFF);
					head.SetTtl(64);
					head.SetPayloadSize(p->GetSize());
					head.SetIdentification(UniformVariable(0, 65536).GetValue());
					p->AddHeader(head);
					uint32_t protocolNumber = 2048;
					AddHeader(p, protocolNumber);	// Attach PPP header
					m_queue->Enqueue(p, 0);
					((*m_ecn_source)[i]).ecnbits = 0;
					((*m_ecn_source)[i]).qfb = 0;
					((*m_ecn_source)[i]).total = 0;
					DequeueAndTransmit();
					Simulator::Schedule(MicroSeconds(m_qcn_interval), &QbbNetDevice::CheckandSendQCN, this, source, qIndex, port);
				}
				else
				{
					((*m_ecn_source)[i]).ecnbits = 0;
					((*m_ecn_source)[i]).qfb = 0;
					((*m_ecn_source)[i]).total = 0;
					Simulator::Schedule(MicroSeconds(m_qcn_interval), &QbbNetDevice::CheckandSendQCN, this, source, qIndex, port);
				}
				break;
			}
			else
			{
				continue;
			}
		}
		return;
	}

	void
		QbbNetDevice::SetBroadcomParams(
			uint32_t pausetime,
			double qcn_interval,
			double qcn_resume_interval,
			double g,
			DataRate minRate,
			DataRate rai,
			uint32_t fastrecover_times
		)
	{
		m_pausetime = pausetime;
		m_qcn_interval = qcn_interval;
		m_rpgTimeReset = qcn_resume_interval;
		m_g = g;
		m_minRate = m_minRate;
		m_rai = rai;
		m_rpgThreshold = fastrecover_times;
	}

	Ptr<Channel>
		QbbNetDevice::GetChannel(void) const
	{
		return m_channel;
	}

	uint32_t
		QbbNetDevice::GetUsedBuffer(uint32_t port, uint32_t qIndex)
	{
		uint32_t i;
		if (m_qcnEnabled)
		{
			for (i = 1; i < m_queue->m_fcount; i++)
			{
				if (m_findex_qindex_map[i] == qIndex && m_findex_udpport_map[i] == port)
					break;
			}
			return m_queue->GetNBytes(i);
		}
		else if (m_timelyEnable)
		{
			for (i = 0; i < m_queue->m_fcount; i++)
			{
				if (m_findex_qindex_map[i] == qIndex && m_findex_udpport_map[i] == port)
					break;
			}
			return m_queue->GetNBytes(i);
		}
		else
		{
			return m_queue->GetNBytes(qIndex);
		}
	}


	void
		QbbNetDevice::SetQueue(Ptr<BEgressQueue> q)
	{
		NS_LOG_FUNCTION(this << q);
		m_queue = q;
	}

	Ptr<BEgressQueue>
		QbbNetDevice::GetQueue()
	{
		return m_queue;
	}


	void
		QbbNetDevice::ResumeECNState(uint32_t inDev)
	{
		m_ECNState[inDev] = 0;
	}

	void
		QbbNetDevice::ResumeECNIngressState(uint32_t inDev)
	{
		m_ECNIngressCount[inDev] = 0;
	}


	void
		QbbNetDevice::ResumeECNEgressState(uint32_t inDev)
	{
		m_ECNEgressCount[inDev] = 0;
	}


	void
		QbbNetDevice::rpr_adjust_rates(uint32_t fIndex, uint32_t hop)
	{
		AdjustRates(fIndex, hop, DataRate("0bps"));
		rpr_fast_recovery(fIndex, hop);
		return;
	}

	void
		QbbNetDevice::rpr_fast_recovery(uint32_t fIndex, uint32_t hop)
	{
		m_rpStage[fIndex][hop] = 1;
		return;
	}


	void
		QbbNetDevice::rpr_active_increase(uint32_t fIndex, uint32_t hop)
	{
		AdjustRates(fIndex, hop, m_rai);
		m_rpStage[fIndex][hop] = 2;
	}


	void
		QbbNetDevice::rpr_active_byte(uint32_t fIndex, uint32_t hop)
	{
		m_rpByteStage[fIndex][hop]++;
		m_txBytes[fIndex][hop] = m_bc;
		rpr_active_increase(fIndex, hop);
	}

	void
		QbbNetDevice::rpr_active_time(uint32_t fIndex, uint32_t hop)
	{
		m_rpTimeStage[fIndex][hop]++;
		m_rpWhile[fIndex][hop] = m_rpgTimeReset;
		Simulator::Cancel(m_rptimer[fIndex][hop]);
		m_rptimer[fIndex][hop] = Simulator::Schedule(MicroSeconds(m_rpWhile[fIndex][hop]), &QbbNetDevice::rpr_timer_wrapper, this, fIndex, hop);
		rpr_active_select(fIndex, hop);
	}


	void
		QbbNetDevice::rpr_fast_byte(uint32_t fIndex, uint32_t hop)
	{
		m_rpByteStage[fIndex][hop]++;
		m_txBytes[fIndex][hop] = m_bc;
		if (m_rpByteStage[fIndex][hop] < m_rpgThreshold)
		{
			rpr_adjust_rates(fIndex, hop);
		}
		else
		{
			rpr_active_select(fIndex, hop);
		}

		return;
	}

	void
		QbbNetDevice::rpr_fast_time(uint32_t fIndex, uint32_t hop)
	{
		m_rpTimeStage[fIndex][hop]++;
		m_rpWhile[fIndex][hop] = m_rpgTimeReset;
		Simulator::Cancel(m_rptimer[fIndex][hop]);
		m_rptimer[fIndex][hop] = Simulator::Schedule(MicroSeconds(m_rpWhile[fIndex][hop]), &QbbNetDevice::rpr_timer_wrapper, this, fIndex, hop);
		if (m_rpTimeStage[fIndex][hop] < m_rpgThreshold)
			rpr_adjust_rates(fIndex, hop);
		else
			rpr_active_select(fIndex, hop);
		return;
	}


	void
		QbbNetDevice::rpr_hyper_byte(uint32_t fIndex, uint32_t hop)
	{
		m_rpByteStage[fIndex][hop]++;
		m_txBytes[fIndex][hop] = m_bc / 2;
		rpr_hyper_increase(fIndex, hop);
	}


	void
		QbbNetDevice::rpr_hyper_time(uint32_t fIndex, uint32_t hop)
	{
		m_rpTimeStage[fIndex][hop]++;
		m_rpWhile[fIndex][hop] = m_rpgTimeReset / 2;
		Simulator::Cancel(m_rptimer[fIndex][hop]);
		m_rptimer[fIndex][hop] = Simulator::Schedule(MicroSeconds(m_rpWhile[fIndex][hop]), &QbbNetDevice::rpr_timer_wrapper, this, fIndex, hop);
		rpr_hyper_increase(fIndex, hop);
	}


	void
		QbbNetDevice::rpr_active_select(uint32_t fIndex, uint32_t hop)
	{
		if (m_rpByteStage[fIndex][hop] < m_rpgThreshold || m_rpTimeStage[fIndex][hop] < m_rpgThreshold)
			rpr_active_increase(fIndex, hop);
		else
			rpr_hyper_increase(fIndex, hop);
		return;
	}


	void
		QbbNetDevice::rpr_hyper_increase(uint32_t fIndex, uint32_t hop)
	{
		AdjustRates(fIndex, hop, m_rhai*(std::min(m_rpByteStage[fIndex][hop], m_rpTimeStage[fIndex][hop]) - m_rpgThreshold + 1));
		m_rpStage[fIndex][hop] = 3;
		return;
	}

	void
		QbbNetDevice::AdjustRates(uint32_t fIndex, uint32_t hop, DataRate increase)
	{
		//	std::cout << "��������" << std::endl;
		if (((m_rpByteStage[fIndex][hop] == 1) || (m_rpTimeStage[fIndex][hop] == 1)) && (m_targetRate[fIndex][hop] > 10 * m_rateAll[fIndex][hop]))
			m_targetRate[fIndex][hop] /= 8;
		else
			m_targetRate[fIndex][hop] += increase;

		m_rateAll[fIndex][hop] = (m_rateAll[fIndex][hop] / 2) + (m_targetRate[fIndex][hop] / 2);

		if (m_rateAll[fIndex][hop] > m_bps)
			m_rateAll[fIndex][hop] = m_bps;

		m_rate[fIndex] = m_bps;
		for (uint32_t j = 0; j < maxHop; j++)
			m_rate[fIndex] = std::min(m_rate[fIndex], m_rateAll[fIndex][j]);
		return;
	}

	void
		QbbNetDevice::rpr_cnm_received(uint32_t findex, uint32_t hop, double fraction)
	{
		if (!m_EcnClampTgtRateAfterTimeInc && !m_EcnClampTgtRate) //Clamp target rate after timer increase. && Clamp target rate.
		{
			if (m_rpByteStage[findex][hop] != 0)  //�ֽڼ�����������
			{
				m_targetRate[findex][hop] = m_rateAll[findex][hop];
				m_txBytes[findex][hop] = m_bc;  //  m_bc: Tx byte counter timeout threshold
			}
		}
		else if (m_EcnClampTgtRate)
		{
			m_targetRate[findex][hop] = m_rateAll[findex][hop];
			m_txBytes[findex][hop] = m_bc; //for fluid model, QCN standard doesn't have this.
		}
		else
		{
			if (m_rpByteStage[findex][hop] != 0 || m_rpTimeStage[findex][hop] != 0)
			{
				m_targetRate[findex][hop] = m_rateAll[findex][hop];
				m_txBytes[findex][hop] = m_bc;
			}
		}
		m_rpByteStage[findex][hop] = 0;  //�ֽڼ����������ڣ��յ�QCNʱ��0
		m_rpTimeStage[findex][hop] = 0;  //��ʱ��������
										 //m_alpha[findex][hop] = (1-m_g)*m_alpha[findex][hop] + m_g*fraction;
		m_alpha[findex][hop] = (1 - m_g)*m_alpha[findex][hop] + m_g; 	//binary feedback
		m_rateAll[findex][hop] = std::max(m_minRate, m_rateAll[findex][hop] * (1 - m_alpha[findex][hop] / 2));
		Simulator::Cancel(m_resumeAlpha[findex][hop]); //ȡ������m_alpha�ĵ��¼�
		m_resumeAlpha[findex][hop] = Simulator::Schedule(MicroSeconds(m_alpha_resume_interval), &QbbNetDevice::ResumeAlpha, this, findex, hop); //���´�������m_alpha���¼�
		m_rpWhile[findex][hop] = m_rpgTimeReset;  //The rate increase timer at RP in microseconds
		Simulator::Cancel(m_rptimer[findex][hop]);
		m_rptimer[findex][hop] = Simulator::Schedule(MicroSeconds(m_rpWhile[findex][hop]), &QbbNetDevice::rpr_timer_wrapper, this, findex, hop);
		rpr_fast_recovery(findex, hop);  //RP���»ص����ٻָ��׶Σ���m_rpStage[fIndex][hop] = 1;
	}

	void
		QbbNetDevice::rpr_timer_wrapper(uint32_t fIndex, uint32_t hop)
	{
		if (m_rpStage[fIndex][hop] == 1)
		{
			rpr_fast_time(fIndex, hop);
		}
		else if (m_rpStage[fIndex][hop] == 2)
		{
			rpr_active_time(fIndex, hop);
		}
		else if (m_rpStage[fIndex][hop] == 3)
		{
			rpr_hyper_time(fIndex, hop);
		}
		return;
	}

	void
		QbbNetDevice::ResumeAlpha(uint32_t fIndex, uint32_t hop)
	{
		m_alpha[fIndex][hop] = (1 - m_g)*m_alpha[fIndex][hop];
		Simulator::Cancel(m_resumeAlpha[fIndex][hop]);
		m_resumeAlpha[fIndex][hop] = Simulator::Schedule(MicroSeconds(m_alpha_resume_interval), &QbbNetDevice::ResumeAlpha, this, fIndex, hop);
	}

	int
		QbbNetDevice::ReceiverCheckSeq(uint32_t seq, uint32_t key)
	{
		uint32_t expected = ReceiverNextExpectedSeq[key];
		if (seq == expected)
		{
			ReceiverNextExpectedSeq[key] = expected + 1;
			if (ReceiverNextExpectedSeq[key] > m_milestone_rx[key] && m_ack_interval > 0)
			{
				m_milestone_rx[key] += m_ack_interval;   //���Ϸ���ACK�ļ��
				return 1; //Generate ACK
			}
			else if (m_chunk != 0 && ReceiverNextExpectedSeq[key] % m_chunk == 0)
			{
				return 1;
			}
			else if (received[key] > rtt_num && m_timelyEnable) {
				return 1;
			}
			else
			{
				return 5;
			}

		}
		else if (seq > expected)
		{
			// Generate NACK
			if (Simulator::Now() > m_nackTimer[key] || m_lastNACK[key] != expected)
			{
				m_nackTimer[key] = Simulator::Now() + MicroSeconds(m_nack_interval);
				m_lastNACK[key] = expected;
				if (m_backto0 && !m_testRead)
				{
					ReceiverNextExpectedSeq[key] = ReceiverNextExpectedSeq[key] / m_chunk*m_chunk;
				}
				return 2;
			}
			else if (received[key] > rtt_num && m_timelyEnable) {
				return 2;
			}
			else
				return 4;
		}
		else
		{
			// Duplicate. 
			return 3;
		}
	}

} // namespace ns3
