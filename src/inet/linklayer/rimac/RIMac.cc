//
// Copyright (C) 2019 Oren Zaharia @ Ben-Gurion University
// Copyright (C) 2017 Jan Peter Drees
// Copyright (C) 2015 Joaquim Oller
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/common/INETUtils.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "inet/linklayer/common/InterfaceTag_m.h"
#include "inet/linklayer/common/MacAddressTag_m.h"
#include "inet/linklayer/rimac/RIMac.h"
#include "inet/linklayer/rimac/RIMacHeader_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"

namespace inet {

using namespace physicallayer;
using namespace std;

Define_Module(RIMac);

/**
 * Initialize method of RIMac. Init all parameters, schedule timers.
 */
void RIMac::initialize(int stage)
{
    MacProtocolBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        animation     = par("animation");
        slotDuration  = par("slotDuration");
        bitrate       = par("bitrate");
        headerLength  = par("headerLength");
        checkInterval = par("checkInterval");
        txPower       = par("txPower");
        useMacAcks    = par("useMACAcks");
        //////////////////////////////////////////////
        //////////////////////////////////////////////
        //////////////////////////////////////////////
        windowSize                  = par("minWindowSize");
        maxTransmissions            = par("maxTransmissions");
        sleepInterval              = par("sleepInterval");
        clearChannelAssessment      = par("clearChannelAssessment");
        sifs                        = par("sifs");



        EV_DEBUG << "headerLength: " << headerLength << ", bitrate: " << bitrate << endl;

        stats = par("stats");
        nbTxDataPackets = 0;
        nbTxPreambles = 0;
        nbRxDataPackets = 0;
        nbRxPreambles = 0;
        nbMissedAcks = 0;
        nbRecvdAcks=0;
        nbDroppedDataPackets=0;
        nbRxBrokenDataPackets = 0;
        nbTxAcks=0;

        txAttempts = 0;
        lastDataPktDestAddr = MacAddress::BROADCAST_ADDRESS;
        lastDataPktSrcAddr  = MacAddress::BROADCAST_ADDRESS;

        macState = INIT;
        txQueue = check_and_cast<queueing::IPacketQueue *>(getSubmodule("queue"));
        WATCH(macState);
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        cModule *radioModule = getModuleFromPar<cModule>(par("radioModule"), this);
        radioModule->subscribe(IRadio::radioModeChangedSignal, this);
        radioModule->subscribe(IRadio::transmissionStateChangedSignal, this);
        radioModule->subscribe(IRadio::receptionStateChangedSignal, this);
        radioModule->subscribe(IRadio::receptionEndedSignal, this);
        radio = check_and_cast<IRadio *>(radioModule);

        wakeup = new cMessage("wakeup");
        wakeup->setKind(RIMAC_WAKE_UP);

        data_timeout = new cMessage("data_timeout");
        data_timeout->setKind(RIMAC_DATA_TIMEOUT);
        data_timeout->setSchedulingPriority(100);

        data_tx_over = new cMessage("data_tx_over");
        data_tx_over->setKind(RIMAC_DATA_TX_OVER);

        stop_preambles = new cMessage("stop_preambles");
        stop_preambles->setKind(RIMAC_STOP_PREAMBLES);

        // Added by Oren
        stop_rx_mode = new cMessage("stop_rx_mode");
        stop_rx_mode->setKind(RIMAC_STOP_RXMODE);

        send_report = new cMessage("send_report");
        send_report->setKind(RIMAC_SEND_REPORT);

        send_preamble = new cMessage("send_preamble");
        send_preamble->setKind(RIMAC_SEND_PREAMBLE);

        ack_tx_over = new cMessage("ack_tx_over");
        ack_tx_over->setKind(RIMAC_ACK_TX_OVER);

        cca_timeout = new cMessage("cca_timeout");
        cca_timeout->setKind(RIMAC_CCA_TIMEOUT);
        cca_timeout->setSchedulingPriority(100);

        send_ack = new cMessage("send_ack");
        send_ack->setKind(RIMAC_SEND_ACK);

        start_rimac = new cMessage("start_rimac");
        start_rimac->setKind(RIMAC_START_RIMAC);

        ack_timeout = new cMessage("ack_timeout");
        ack_timeout->setKind(RIMAC_ACK_TIMEOUT);

        resend_data = new cMessage("resend_data");
        resend_data->setKind(RIMAC_RESEND_DATA);
        resend_data->setSchedulingPriority(100);

        switch_preamble_phase = new cMessage("switch_preamble_phase");
        switch_preamble_phase->setKind(SWITCH_PREAMBLE_PHASE);

        report_timeout = new cMessage("report_timeout");
        report_timeout->setKind(RIMAC_REPORT_TIMEOUT);

        report_wait_to = new cMessage("report_wait_to");
        report_wait_to->setKind(RIMAC_WAIT_TIMEOUT);

        report_preamble_to = new cMessage("report_preamble_to");
        report_preamble_to->setKind(RIMAC_PREAMBLE_TIMEOUT);

        report_msg = new cMessage("report_msg");
        report_msg->setKind(RIMAC_REPORT_MSG);

        delay_for_ack_within_remote_rx = new cMessage("delay_for_ack_within_remote_rx");
        delay_for_ack_within_remote_rx->setKind(DELAY_FOR_ACK_WITHIN_REMOTE_RX);

        switching_done = new cMessage("switching_done");
        switching_done->setKind(RIMAC_SWITCHING_FINISHED);

        rimac_cs = new cMessage("rimac_cs");
        rimac_cs->setKind(RIMAC_CARRIERSENSE);

        rimac_start_cca = new cMessage("rimac_start_cca");
        rimac_start_cca->setKind(RIMAC_START_CCA);

        rimac_col_ended = new cMessage("rimac_col_ended");
        rimac_col_ended->setKind(RIMAC_COLLISION_ENDED);

        rimac_backoff_to = new cMessage("rimac_backoff_to");
        rimac_backoff_to->setKind(RIMAC_BACKOFF_TO);

        rimac_rcv_start = new cMessage("rimac_rcv_start");
        rimac_rcv_start->setKind(RIMAC_RCV_STARTED);

        rimac_beacon_end = new cMessage("rimac_beacon_end");
        rimac_beacon_end->setKind(RIMAC_BEACON_ENDED);


        auto preamble = makeShared<RIMacHeader>();
        preamble->setSrcAddr(interfaceEntry->getMacAddress());
        preamble->setChunkLength(b(headerLength));
        preamble->setType(RIMAC_PREAMBLE);

        auto packet = new Packet("Preamble", preamble);

        lastPreamblePktSent = packet;

        scheduleAt(simTime(), start_rimac);
    }
}

RIMac::~RIMac()
{
    cancelAndDelete(wakeup);
    cancelAndDelete(data_timeout);
    cancelAndDelete(data_tx_over);
    cancelAndDelete(stop_preambles);
    cancelAndDelete(send_preamble);
    cancelAndDelete(ack_tx_over);
    cancelAndDelete(cca_timeout);
    cancelAndDelete(send_ack);
    cancelAndDelete(start_rimac);
    cancelAndDelete(ack_timeout);
    cancelAndDelete(resend_data);
    cancelAndDelete(switch_preamble_phase);
    cancelAndDelete(delay_for_ack_within_remote_rx);
    cancelAndDelete(switching_done);
    cancelAndDelete(stop_rx_mode);

    cancelAndDelete(report_msg);
    cancelAndDelete(report_preamble_to);
    cancelAndDelete(report_wait_to);
    cancelAndDelete(rimac_cs);
    cancelAndDelete(rimac_start_cca);
    cancelAndDelete(send_report);
    cancelAndDelete(report_timeout);
    cancelAndDelete(rimac_col_ended);
    cancelAndDelete(rimac_backoff_to);
    cancelAndDelete(rimac_rcv_start);

    delete lastPreamblePktSent;

}

void RIMac::finish()
{
    // record stats
    if (stats) {
        recordScalar("nbTxDataPackets", nbTxDataPackets);
        recordScalar("nbTxPreambles", nbTxPreambles);
        recordScalar("nbRxDataPackets", nbRxDataPackets);
        recordScalar("nbRxPreambles", nbRxPreambles);
        recordScalar("nbMissedAcks", nbMissedAcks);
        recordScalar("nbRecvdAcks", nbRecvdAcks);
        recordScalar("nbTxAcks", nbTxAcks);
        recordScalar("nbDroppedDataPackets", nbDroppedDataPackets);
        recordScalar("nbRxBrokenDataPackets", nbRxBrokenDataPackets);
        //recordScalar("timeSleep", timeSleep);
        //recordScalar("timeRX", timeRX);
        //recordScalar("timeTX", timeTX);
    }
}

void RIMac::configureInterfaceEntry()
{
    MacAddress address = parseMacAddressParameter(par("address"));

    // data rate
    interfaceEntry->setDatarate(bitrate);

    // generate a link-layer address to be used as interface token for IPv6
    interfaceEntry->setMacAddress(address);
    interfaceEntry->setInterfaceToken(address.formInterfaceIdentifier());

    // capabilities
    interfaceEntry->setMtu(par("mtu"));
    interfaceEntry->setMulticast(false);
    interfaceEntry->setBroadcast(true);


}

/**
 * Check whether the queue is not full: if yes, print a warning and drop the
 * packet. Then initiate sending of the packet, if the node is sleeping. Do
 * nothing, if node is working.
 */
void RIMac::handleUpperPacket(Packet *packet)
{
    encapsulate(packet);
    EV_DETAIL << "CSMA received a message from upper layer, name is " << packet->getName() << ", CInfo removed, mac addr=" << packet->peekAtFront<RIMacHeader>()->getDestAddr() << endl;
    EV_DETAIL << "pkt encapsulated, length: " << packet->getBitLength() << "\n";
    txQueue->pushPacket(packet);
    EV_DEBUG << "Max queue length: " << txQueue->getMaxNumPackets() << ", packet put in queue"
              "\n  queue size: " << txQueue->getNumPackets() << " macState: "
              << macState << endl;
    // force wakeup now
    if (false && !txQueue->isEmpty() && wakeup->isScheduled() && (macState == SLEEP))
    {
        cancelEvent(wakeup);
        scheduleAt(simTime() + dblrand()*0.01f, wakeup);
    }
}

/**
 * Send one short preamble packet immediately.
 */
void RIMac::sendPreamble(MacAddress preamble_address)
{
    //~ diff with RIMac, @ in preamble!
    EV << "Preamble creation Oren" << endl;
    auto preamble = makeShared<RIMacHeader>();
    preamble->setSrcAddr(interfaceEntry->getMacAddress());
    EV << "SRC Addr: " << interfaceEntry->getMacAddress() << endl;
    preamble->setDestAddr(preamble_address);
    EV << "DST Addr: " << preamble_address << endl;
    preamble->setChunkLength(b(headerLength));
    EV << "Header Length: " << b(headerLength) << endl;
    preamble->setType(RIMAC_PREAMBLE);
    EV << "Type: " << RIMAC_PREAMBLE << endl;
    auto packet = new Packet("Preamble", preamble);
    packet->addTag<PacketProtocolTag>()->setProtocol(&Protocol::rimac);
    attachSignal(packet, simTime());
    sendDown(packet);
    nbTxPreambles++;
}
/**
 * Send one short preamble packet immediately.
 */
void RIMac::sendPreamble()
{
    //~ diff with RIMac, @ in preamble!
    /*
    auto preamble = makeShared<RIMacHeader>();
    preamble->setSrcAddr(interfaceEntry->getMacAddress());
    preamble->setChunkLength(b(headerLength));
    preamble->setType(RIMAC_PREAMBLE);
    */
    //auto packet = new Packet("Preamble", preamble);
    auto packet = lastPreamblePktSent->dup();
    packet->addTag<PacketProtocolTag>()->setProtocol(&Protocol::rimac);
    attachSignal(packet, simTime());
    sendDown(packet);
    nbTxPreambles++;
}
/**
 * Send one short preamble packet immediately.
 */
void RIMac::sendWindow(int windowSize)
{
    //~ diff with RIMac, @ in preamble!
    auto beacon = makeShared<RIMacHeader>();
    beacon->setSrcAddr(interfaceEntry->getMacAddress());
    beacon->setChunkLength(b(headerLength + 3));
    beacon->setType(RIMAC_BEACON);
    beacon->setWindowSize(windowSize);

    auto beaconPkt = new Packet("Beacon", beacon);
    beaconPkt->addTag<PacketProtocolTag>()->setProtocol(&Protocol::rimac);
    attachSignal(beaconPkt, simTime());
    sendDown(beaconPkt);
    //nbTxPreambles++;
}
/**
 * Send one short preamble packet immediately.
 */
void RIMac::sendMacAck()
{
    auto ack = makeShared<RIMacHeader>();
    ack->setSrcAddr(interfaceEntry->getMacAddress());
    EV << "Oren setSrcAddr:" << interfaceEntry->getMacAddress()<< endl;
    //~ diff with RIMac, ack_preamble_based
    ack->setDestAddr(lastPreamblePktSrcAddr);
    EV << "Oren setDestAddr:" << lastPreamblePktSrcAddr<< endl;
    ack->setChunkLength(b(headerLength));
    EV << "Oren setChunkLength:" <<b(headerLength) << endl;
    ack->setType(RIMAC_ACK);
    EV << "Oren setType:" << RIMAC_ACK << endl;
    auto packet = new Packet("RIMacAck", ack);
    packet->addTag<PacketProtocolTag>()->setProtocol(&Protocol::rimac);
    attachSignal(packet, simTime());
    sendDown(packet);
    nbTxAcks++;
}


/**
 * Handle own messages:
 */
void RIMac::handleSelfMessage(cMessage *msg)
{
    MacAddress address = interfaceEntry->getMacAddress();



    switch (macState)
    {
    case INIT:
        if (msg->getKind() == RIMAC_START_RIMAC)
        {
            EV << address << endl;
            cout << address << endl;
            EV_DEBUG << "State INIT, message RIMAC_START, new state SLEEP" << endl;
            changeDisplayColor(BLACK);
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            macState = SLEEP;
            double rand = dblrand();

            scheduleAt(simTime()+rand*sleepInterval, wakeup);

            return;
        }
        break;
    case SLEEP:
        //when wakeup, go immediately to CCA state
        if (msg->getKind() == RIMAC_WAKE_UP)
        {
            EV_DEBUG << "node " << address << " : State SLEEP, message RIMAC_WAKEUP, new state CCA, simTime " <<
                    simTime() << " to " << simTime() + clearChannelAssessment << endl;

            scheduleAt(simTime() + sleepInterval, wakeup);
            this->rcvWindowSize = 0;
            macState = CCA;
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            scheduleAt(simTime() + sifs, rimac_start_cca);
            changeDisplayColor(RED);


            return;
        }
        else
        {
            // I got a message, but I'm sleep, we should ignore it
            // TODO: add some statistics before the ignore=return
            return;
        }
        break;
    case CCA:
        if (msg->getKind() == RIMAC_START_CCA)
        {
            if (radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING || radio->getReceptionState() == IRadio::RECEPTION_STATE_BUSY)
            {
                // Go to sleep
                // the medium is in used while we're in CCA
                auto origTime = wakeup->getArrivalTime();
                cancelEvent(wakeup);
                double  rand = uniform(0.1, 0.5);
                cout << "CCA slot offset: " << rand * sleepInterval << endl;
                scheduleAt(origTime +  rand * sleepInterval, wakeup);

                macState = SLEEP;
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                changeDisplayColor(BLACK);
                return;
            }
            else
            {
                macState = CCA; // we are already in CCA, this is not neccasary, but logic.
                scheduleAt(simTime() + clearChannelAssessment, cca_timeout);
                return;
            }
        }
        else if (msg->getKind() == RIMAC_CCA_TIMEOUT)
        {
            //CCA is timed out, we may send a preamble that we are awake
            // go to SEND_PREAMBLE
            macState = SEND_PREAMBLE;
            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            scheduleAt(simTime() + sifs, send_preamble);
            changeDisplayColor(YELLOW);
            return;
        }
        else if (msg->getKind() == RIMAC_CARRIERSENSE)
        {

            // Check if the queue status
            // if queue is empty, go to sleep
            // if queue is not empty change state to WAIT_PREAMBLE

            if (txQueue->getNumPackets())
            {

                // move the slotDuration, such that the next CCA won't sense again
                auto origTime = wakeup->getArrivalTime();
                cancelEvent(wakeup);
                double  rand = uniform(0.1, 0.5);
                cout << "CCA slot offset: " << rand * slotDuration << endl;
                scheduleAt(origTime +  rand * sleepInterval, wakeup);
                scheduleAt(simTime() + 1.0f*slotDuration, report_preamble_to);
                macState = WAIT_PREAMBLE;
                changeDisplayColor(GREEN);
                if (currentTxFrame == nullptr)
                {
                    popTxQueue();
                }
                return;
            }
            else
            {
                auto origTime = wakeup->getArrivalTime();
                cancelEvent(wakeup);
                double  rand = uniform(0.1, 0.5);
                cout << "CCA slot offset: " << rand * sleepInterval << endl;
                scheduleAt(origTime +  rand * sleepInterval, wakeup);
                macState = SLEEP;
                changeDisplayColor(BLACK);
                return;
            }
            /*

            */
        }
        else if (msg->getKind() == RIMAC_PREAMBLE)
        {
            // TODO: check if the message for me
            if (hasGUI())
            {
                char text[32];
                sprintf(text, "CS!");
                this->getParentModule()->getParentModule()->bubble(text);
            }
            auto origTime = wakeup->getArrivalTime();
            cancelEvent(wakeup);
            double  rand = uniform(0.1, 0.5);
            cout << "CCA slot offset: " << rand * sleepInterval << endl;
            scheduleAt(origTime +  rand * sleepInterval, wakeup);
            macState = SLEEP;
            changeDisplayColor(BLACK);
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            return;
        }
        else if (msg->getKind() == RIMAC_REPORT_MSG)
        {
            if (hasGUI())
            {
                char text[32];
                sprintf(text, "CS!");
                this->getParentModule()->getParentModule()->bubble(text);
            }
            // TODO: check if the message for me, improbable scenario
            auto origTime = wakeup->getArrivalTime();
            cancelEvent(wakeup);
            double  rand = uniform(0.1, 0.5);
            cout << "CCA slot offset: " << rand * sleepInterval << endl;
            scheduleAt(origTime +  rand * sleepInterval, wakeup);
            macState = SLEEP;
            changeDisplayColor(BLACK);
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            return;

        }
        break;
    case SEND_PREAMBLE:
        if (msg->getKind() == RIMAC_SEND_PREAMBLE)
        {
            if (radio->getRadioMode() == IRadio::RADIO_MODE_TRANSMITTER)
            {
                macState = WAIT_BEACON_TX_END;
                changeDisplayColor(YELLOW);
                changeColor("green");
                sendPreamble();

                return;
            }
        }
        else if (msg->getKind() == RIMAC_DATA_TX_OVER)
        {
            // we've finished transmistting the preamble
            macState = WAIT_REPORT;
            scheduleAt(simTime() + sifs, switch_preamble_phase);
            changeDisplayColor(YELLOW);
            return;
        }
        return;
        break;
    case WAIT_REPORT:
        if (msg->getKind() == RIMAC_WAIT_TIMEOUT)
        {
            // Check queue
            // if queue is empty, go to sleep and wakeup by the original scheduler
            // otherwise wait to preamble a slot duration
            if (txQueue->getNumPackets()) /* pop() == 1 */
            {
                macState = WAIT_PREAMBLE;
                changeDisplayColor(GREEN);
                scheduleAt(simTime() + 1.0f*sleepInterval, report_preamble_to);
                if (currentTxFrame == nullptr)
                {
                    popTxQueue();
                }
                return;
            }
            else /* pop() == 0 */
            {
                // I don't have something to send
                macState = SLEEP;
                changeDisplayColor(BLACK);
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                isBackoff = false;
                windowSize = par("minWindowSize");
                return;
            }

        }
        else if (msg->getKind() == SWITCH_PREAMBLE_PHASE)
        {
            radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
            if (!isBackoff)
            {
                double maxPropagation = par("maxRange").doubleValue() / SPEED_OF_LIGHT;
                if(report_wait_to->isScheduled())
                {
                    cancelEvent(report_wait_to);
                }
                scheduleAt(simTime() + 2.0f*sifs + clearChannelAssessment + 2.5f * maxPropagation, report_wait_to);
            }
            else
            {
                if(report_wait_to->isScheduled())
                {
                    cancelEvent(report_wait_to);
                }
                scheduleAt(simTime() + 2.0f*sifs + clearChannelAssessment + 1.0f*slotDuration*(1 << windowSize), report_wait_to);
            }
            return;
        }
        else if (msg->getKind() == RIMAC_REPORT_MSG)
        {
            auto packet = check_and_cast<Packet *>(msg);
            auto mac = packet->peekAtFront<RIMacHeader>();
            cout << "Packas Data has arrived" << endl;
            cout << "Radio reception state: "<< radio->getReceptionState() << endl;
            if (packet->hasBitError())
            {
                cout << "packet->hasBitError()" << endl;
            }
            else if (!packet->peekData()->isCorrect())
            {
                cout << "!packet->peekData()->isCorrect()" << endl;
            }
            const MacAddress& dest = mac->getDestAddr();
            decapsulate(packet);
            sendUp(packet);
            nbRxDataPackets++;
            cout << "Received Data packet" << endl;
            if (report_wait_to->isScheduled())
            {
                cancelEvent(report_wait_to);
            }
            scheduleAt(simTime(), send_preamble);
            return;

        }
        else if (msg->getKind() == RIMAC_SEND_PREAMBLE)
        {
            if (!isBackoff)
            {
                macState = SEND_BEACON;
                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
                scheduleAt(simTime() + sifs, send_preamble);
            }
            else
            {
                macState = SEND_BEACON_W;
                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

                scheduleAt(simTime() + sifs, send_preamble);
            }
            changeDisplayColor(YELLOW);
            return;
        }
        else if (msg->getKind() == RIMAC_COLLISION_ENDED)
        {
            // macState is WAIT_REPORT
            // Two or more tranmission has been made.
            // We need to send a beacon with backoff window
            macState = SEND_BEACON_W;
            (this->windowSize)++;
            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            changeDisplayColor(YELLOW);
            scheduleAt(simTime() + sifs, send_preamble);
            return;
        }
        else if (msg->getKind() == RIMAC_RCV_STARTED)
        {
            // macState is WAIT_REPORT
            if (report_wait_to->isScheduled())
            {

                cancelEvent(report_wait_to);
            }

            return;
        }
        else if (msg->getKind() == RIMAC_PREAMBLE)
        {
            // macState is WAIT_REPORT
            // Good for star topology ONLY
            macState = SEND_BEACON_W;
            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            changeDisplayColor(YELLOW);
            scheduleAt(simTime() + sifs, send_preamble);
            return;
        }
        else if (msg->getKind() == RIMAC_BEACON)
        {
            // macState is WAIT_REPORT
            // Good for star topology ONLY
            macState = SLEEP;
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            changeDisplayColor(BLACK);
            return;
        }
        break;
    case SEND_BEACON:
        if (msg->getKind() == RIMAC_SEND_PREAMBLE)
        {
            if (radio->getRadioMode() == IRadio::RADIO_MODE_TRANSMITTER)
            {
                // TODO: check windowSize, there is a maximum
                // TODO: do we need here a maxPropagation add of time?
                isBackoff = false;
                macState = WAIT_BEACON_TX_END;
                changeDisplayColor(YELLOW);
                sendWindow(0);
                return;
            }
        }
        break;
    case SEND_BEACON_W:
        if (msg->getKind() == RIMAC_SEND_PREAMBLE)
        {
            if (radio->getRadioMode() == IRadio::RADIO_MODE_TRANSMITTER)
            {
                // TODO: check windowSize, there is a maximum
                // TODO: do we need here a maxPropagation add of time?
                isBackoff = true;
                macState = WAIT_BEACON_TX_END;
                changeDisplayColor(YELLOW);
                sendWindow(this->windowSize);
                return;
            }
        }
        break;
    case WAIT_BEACON_TX_END:
        if (msg->getKind() == RIMAC_BEACON_ENDED)
        {
            // TODO: check windowSize, there is a maximum
            // TODO: do we need here a maxPropagation add of time?
            macState = WAIT_REPORT;
            scheduleAt(simTime() + sifs, switch_preamble_phase);
            changeDisplayColor(YELLOW);
            return;
        }
        break;
    case WAIT_PREAMBLE:
        if (msg->getKind() == RIMAC_WAKE_UP)
        {
            scheduleAt(simTime() + sleepInterval, wakeup);
            cout << "WAIT_PREAMBLE" << endl;
            cout << "wakeup->isScheduled() = "<< wakeup->isScheduled() << endl;
            cout << "wakeup->getArrivalTime() = "<< wakeup->getArrivalTime() << endl;
            return;
        }
        else if (msg->getKind() == RIMAC_PREAMBLE)
        {
            // We got preamble, is it for us?
            // if we have REPORT for this node, send it, otherwise, ignore it.
            auto incoming_data = check_and_cast<Packet *>(msg)->peekAtFront<RIMacHeader>();
            auto tx_frame = currentTxFrame->peekAtFront<RIMacHeader>();
            if (incoming_data->getSrcAddr() != tx_frame->getDestAddr())
            {
                //ignore, that is return
                EV << "node " << address << " : State WAIT_PREAMBLE, received RIMAC_PREAMBLE, not for me, IGNORE" << endl;
                return;
            }

            // we have been arrived from WAIT_REPORT after a timeout

            EV << "node " << address << " : State WAIT_PREAMBLE, received RIMAC_PREAMBLE" << endl;
            //auto incoming_data = check_and_cast<Packet *>(msg)->peekAtFront<RIMacHeader>();
            //auto tx_frame = currentTxFrame->peekAtFront<RIMacHeader>();
            cout << incoming_data->getSrcAddr() << endl;
            cout << tx_frame->getDestAddr() << endl;
            cout << (incoming_data->getSrcAddr() == tx_frame->getDestAddr()) << endl;

            if (incoming_data->getSrcAddr() == tx_frame->getDestAddr())
            {
                //send the tx_frame
                EV << "node " << address << " : State WAIT_PREAMBLE, received RIMAC_PREAMBLE, new state SEND_REPORT" << endl;
                macState = SEND_REPORT;
                changeDisplayColor(YELLOW);
                radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);

                scheduleAt(simTime() + sifs, send_report);
                return;
            }
            return;
        }
        else if (msg->getKind() == RIMAC_PREAMBLE_TIMEOUT)
        {
            if (cntTx + txAttempts < maxTransmissions && txQueue->getNumPackets()) // pop() == 1
            {
                txAttempts++;
                if (currentTxFrame == nullptr)
                {
                    popTxQueue();
                }
                scheduleAt(simTime() + sleepInterval, report_preamble_to);
                return;
            }
            else
            {
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                macState = SLEEP;
                changeDisplayColor(BLACK);
                return;
            }
        }
        else if (msg->getKind() == SWITCH_PREAMBLE_PHASE)
        {
            scheduleAt(simTime() + sleepInterval, report_preamble_to);
            return;
        }
        else if (msg->getKind() == RIMAC_BEACON)
        {
            auto incoming_data = check_and_cast<Packet *>(msg)->peekAtFront<RIMacHeader>();
            auto tx_frame = currentTxFrame->peekAtFront<RIMacHeader>();

            if (incoming_data->getSrcAddr() != tx_frame->getDestAddr())
            {
                //ignore, that is return
                EV << "node " << address << " : State WAIT_PREAMBLE, received RIMAC_PREAMBLE, not for me, IGNORE" << endl;
                return;
            }
            if (tx_frame->getWindowSize() == 0)
            {
                //send the tx_frame
                EV << "node " << address << " : State WAIT_PREAMBLE, received RIMAC_PREAMBLE, new state SEND_REPORT" << endl;
                macState = SEND_REPORT;
                changeDisplayColor(YELLOW);

                scheduleAt(simTime() + sifs, switch_preamble_phase);
                return;
            }
            else if (tx_frame->getWindowSize() > 0)
            {
                // there was or still collision, give up and go to sleep
                cancelEvent(report_preamble_to);
                macState = SLEEP;
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                changeDisplayColor(BLACK);
                return;

            }
            return;
        }
        break;
    case SEND_REPORT:
        if (msg->getKind() == SWITCH_PREAMBLE_PHASE)
        {
            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            scheduleAt(simTime(), send_report);
            return;
        }
        else if (msg->getKind() == RIMAC_SEND_REPORT)
        {
            auto tx_frame = currentTxFrame->peekAtFront<RIMacHeader>();
            cancelEvent(report_preamble_to);

            sendReportPacket();
            macState = WAIT_TX_DATA_OVER;
            changeDisplayColor(GREEN);
            cntTx++;
            return;
        }
        break;
    case WAIT_TX_DATA_OVER:
            if (msg->getKind() == RIMAC_DATA_TX_OVER)
            {
                // Transmistting has been ended
                // go to new scenario
                macState = WAIT_ACK;
                scheduleAt(simTime() + sifs, switch_preamble_phase);
                radio->setRadioMode(IRadio::RADIO_MODE_RECEIVER);
                changeDisplayColor(GREEN);
            }
            else if (msg->getKind() == RIMAC_WAKE_UP)
            {
                scheduleAt(simTime() + sleepInterval, wakeup);
                cout << "WAIT_TX_DATA_OVER" << endl;
                cout << "wakeup->isScheduled() = "<< wakeup->isScheduled() << endl;
                cout << "wakeup->getArrivalTime() = "<< wakeup->getArrivalTime() << endl;
                return;
            }
            break;
    case WAIT_ACK:
        if (msg->getKind() == RIMAC_WAKE_UP)
        {
            scheduleAt(simTime() + sleepInterval, wakeup);
            cout << "WAIT_ACK" << endl;
            cout << "wakeup->isScheduled() = "<< wakeup->isScheduled() << endl;
            cout << "wakeup->getArrivalTime() = "<< wakeup->getArrivalTime() << endl;

            return;
        }
        else if (msg->getKind() == SWITCH_PREAMBLE_PHASE)
        {
            // 2.0f * 1.7f max propagation
            scheduleAt(simTime() + 2.0f*sifs, report_preamble_to);
            return;
        }
        else if (msg->getKind() == RIMAC_BEACON)
        {
            auto incoming_data = check_and_cast<Packet *>(msg)->peekAtFront<RIMacHeader>();
            int windowSize = incoming_data->getWindowSize();
            // TODO: the beacon is not for me
            if (windowSize == 0)
            {
                deleteCurrentTxFrame();
                if (txQueue->getNumPackets())
                {
                    cout << "I'm " << this->getParentModule()->getParentModule()->getFullName() << " and I've "<< txQueue->getNumPackets() << " msgs" << endl;
                    popTxQueue();
                    cout << "I'm " << this->getParentModule()->getParentModule()->getFullName() << " and I've "<< txQueue->getNumPackets() << " msgs" << endl;
                    auto tx_frame = currentTxFrame->peekAtFront<RIMacHeader>();
                    if (incoming_data->getSrcAddr() == tx_frame->getDestAddr())
                    {
                        // Go immediately to SEND_REPORT
                        macState = SEND_REPORT;
                        changeDisplayColor(YELLOW);
                        radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
                        //double switchingTime = par("switchingTime"); TODO
                        scheduleAt(simTime() + sifs, send_report);
                        return;
                    }
                    else
                    {
                        // Go to WAIT_PREAMBLE with ordinary timeout
                        // ACK_0_NEW_DST
                        macState = WAIT_PREAMBLE;
                        changeDisplayColor(YELLOW);
                        scheduleAt(simTime() + 1.0f*slotDuration, report_preamble_to);
                        return;
                    }

                }
                else
                {
                    // Go to sleep
                    macState = SLEEP;
                    changeDisplayColor(YELLOW);
                    radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                    return;
                }
            }
            else if (windowSize == this->rcvWindowSize)
            {
                // This is a ACK, but there is a congestion, let them transmit their data

                deleteCurrentTxFrame();
                // Go to sleep too
                macState = SLEEP;
                changeDisplayColor(YELLOW);
                radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
                return;
            }
            else
            {
                // NACK, there is a congestion
                this->rcvWindowSize = windowSize;
                int backoff    = intuniform(0, 1 << windowSize);

                cout << "max: " << (1 << windowSize) << endl;
                cout << "backoff: " << (backoff) << endl;
                macState = WAIT_BACKOFF;
                changeDisplayColor(YELLOW);
                // Backoff for 10.0f check interval
                //scheduleAt(simTime() + 1.0f * backoff * slotDuration, send_report);
                scheduleAt(simTime() + 1.0f * backoff * slotDuration, switch_preamble_phase);
            }
            return;
        }
        else if (msg->getKind() == RIMAC_REPORT_TIMEOUT)
        {
            // What to do?
            // Go sleep
            macState = SLEEP;
            changeDisplayColor(BLACK);
            radio->setRadioMode(IRadio::RADIO_MODE_SLEEP);
            return;
        }
        break;
    case WAIT_BACKOFF:
        if (msg->getKind() == SWITCH_PREAMBLE_PHASE)
        {
            scheduleAt(simTime() + sifs, send_report);
            return;
        }
        else if (msg->getKind() == RIMAC_SEND_REPORT)
        {
            macState = SEND_REPORT;
            changeDisplayColor(YELLOW);
            radio->setRadioMode(IRadio::RADIO_MODE_TRANSMITTER);
            scheduleAt(simTime(), send_report);
            return;
        }
        else if (msg->getKind() == RIMAC_BEACON)
        {
            // This scenario is impposible
            // TODO: add to receptionStateChanged, change macState to WAIT_BEACON if BUSY or RECEIVING
            return;
        }
        else if (msg->getKind() == RIMAC_WAKE_UP)
        {
            scheduleAt(simTime() + sleepInterval, wakeup);

            return;
        }
        break;
    case WAIT_BEACON:
        cout << "WAIT_BEACON rcv: " << msg->getKind() << endl;
        if (msg->getKind() == RIMAC_WAKE_UP)
        {
            scheduleAt(simTime() + sleepInterval, wakeup);

            return;
        }
        else if (msg->getKind() == RIMAC_BEACON)
        {
            // Received BEACON
            auto incoming_data = check_and_cast<Packet *>(msg)->peekAtFront<RIMacHeader>();
            int windowSize = incoming_data->getWindowSize();
            this->rcvWindowSize = windowSize;
            int backoff = intuniform(0, 1 << windowSize);

            if (send_report->isScheduled())
            {
                cancelEvent(send_report);
            }
            if (switch_preamble_phase->isScheduled())
            {
                cancelEvent(switch_preamble_phase);
            }
            macState = WAIT_BACKOFF;
            changeDisplayColor(GREEN);

            scheduleAt(simTime() + 1.0f * slotDuration * backoff, switch_preamble_phase);

            return;
        }
        break;
    throw cRuntimeError("Undefined event of type %d in state %d (Radio state %d)!",
            msg->getKind(), macState, radio->getRadioMode());
    }
}


/**
 * Handle RIMAC preambles and received data packets.
 */
void RIMac::handleLowerPacket(Packet *msg)
{
    if (msg->hasBitError()) {
        EV << "Received " << msg << " contains bit errors or collision, dropping it\n";

        handleErrorInMessage();
        nbRxBrokenDataPackets++;

        delete msg;
        return;
    }
    // simply pass the massage as self message, to be processed by the FSM.
    const auto& hdr = msg->peekAtFront<RIMacHeader>();
    msg->setKind(hdr->getType());
    handleSelfMessage(msg);
}

void RIMac::handleErrorInMessage(void)
{
    switch (macState)
    {
    case WAIT_REPORT:
        if (radio->getReceptionState() != IRadio::RECEPTION_STATE_IDLE)
        {
            isErrorDetected = true;
        }
        else
        {
            scheduleAt(simTime(), rimac_col_ended);
        }
        break;
    default:
        break;
    }
}
void RIMac::sendDataPacket()
{
    nbTxDataPackets++;
    if (currentTxFrame == nullptr)
        popTxQueue();
    auto packet = currentTxFrame->dup();
    const auto& hdr = packet->peekAtFront<RIMacHeader>();
    lastDataPktDestAddr = hdr->getDestAddr();
    ASSERT(hdr->getType() == RIMAC_DATA);
    attachSignal(packet, simTime());
    sendDown(packet);
}

void RIMac::sendReportPacket()
{
    nbTxDataPackets++;
    auto packet = currentTxFrame->dup();
    const auto& hdr = packet->peekAtFront<RIMacHeader>();
    lastDataPktDestAddr = hdr->getDestAddr();
    ASSERT(hdr->getType() == RIMAC_REPORT_MSG);
    attachSignal(packet, simTime());
    sendDown(packet);
}
/**
 * Handle transmission over messages: either send another preambles or the data
 * packet itself.
 */
void RIMac::receiveSignal(cComponent *source, simsignal_t signalID, long value, cObject *details)
{
    Enter_Method_Silent();

    if (signalID == IRadio::receptionStateChangedSignal)
    {
        if (macState == WAIT_REPORT)
        {

            if (radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE)
            {
                if (isErrorDetected)
                {
                    // That is, collision has been detected and now the medium is free.
                    if (hasGUI())
                    {
                        char text[32];
                        sprintf(text, "Collision!");
                        this->getParentModule()->getParentModule()->bubble(text);
                    }
                    isErrorDetected = false;
                    scheduleAt(simTime(), rimac_col_ended);
                }
                else if (receptionState == IRadio::RECEPTION_STATE_BUSY)
                {
                    // We have change our reception mode from BUSY to IDLE.
                    // That is, collision has been detected and now the medium is free.
                    if (hasGUI())
                    {
                        char text[32];
                        sprintf(text, "Collision!");
                        this->getParentModule()->getParentModule()->bubble(text);
                    }
                    scheduleAt(simTime(), rimac_col_ended);

                }
                if (receptionState == IRadio::RECEPTION_STATE_RECEIVING)
                {

                }
            }
            if (radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING)
            {
                receptionState = radio->getReceptionState();
                scheduleAt(simTime(), rimac_rcv_start);
            }

        }
        else if (macState == WAIT_BACKOFF)
        {
            if (radio->getReceptionState() == IRadio::RECEPTION_STATE_BUSY)
            {

                macState = WAIT_BEACON;
                changeDisplayColor(RED);
                cancelEvent(switch_preamble_phase);
                cancelEvent(send_report);
            }
            else if (radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING)
            {
                macState = WAIT_BEACON;
                changeDisplayColor(RED);
                cancelEvent(switch_preamble_phase);
                cancelEvent(send_report);
            }
        }
        else if (macState == WAIT_PREAMBLE)
        {
            if (radio->getReceptionState() == IRadio::RECEPTION_STATE_BUSY)
            {
                cancelEvent(report_preamble_to);
            }
            else if (radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING)
            {
                cancelEvent(report_preamble_to);
            }
            return;
        }
        else
        {
            if (radio->getReceptionState() == IRadio::RECEPTION_STATE_BUSY)
            {
                //The radio medium is busy, a signal is detected but it is not strong enough to receive.
                scheduleAt(simTime(), rimac_cs);
                return;
            }
            else if (radio->getReceptionState() == IRadio::RECEPTION_STATE_RECEIVING)
            {
                //The radio medium is busy, a signal strong enough to receive is detected.
                scheduleAt(simTime(), rimac_cs);
                return;
            }
            else if (radio->getReceptionState() == IRadio::RECEPTION_STATE_IDLE)
            {
                //The radio medium is free, no signal is detected.
            }
            else if (radio->getReceptionState() == IRadio::RECEPTION_STATE_UNDEFINED)
            {
                //The radio medium state is unknown, reception state is meaningless, signal detection is not possible...
            }
        }
        receptionState = radio->getReceptionState();
        return;
    }
    else if (signalID == IRadio::transmissionStateChangedSignal)
    {
        IRadio::TransmissionState newRadioTransmissionState = (IRadio::TransmissionState)value;
        if (transmissionState == IRadio::TRANSMISSION_STATE_TRANSMITTING && newRadioTransmissionState == IRadio::TRANSMISSION_STATE_IDLE)
        {
            // Transmission of one packet is over
            if (macState == WAIT_TX_DATA_OVER) {
                scheduleAt(simTime(), data_tx_over);
            }
            if (macState == WAIT_ACK_TX) {
                scheduleAt(simTime(), ack_tx_over);
            }
            if (macState == SEND_PREAMBLE)
            {
                scheduleAt(simTime(), data_tx_over);
            }
            if (macState == WAIT_BEACON_TX_END)
            {
                scheduleAt(simTime(), rimac_beacon_end);
            }
        }
        transmissionState = newRadioTransmissionState;
    }
    else if (signalID ==IRadio::radioModeChangedSignal) {
        // Radio switching (to RX or TX) is over, ignore switching to SLEEP.
        if (false && macState == SEND_REPORT)
        {
            scheduleAt(simTime(), send_report);
        }
        else if (macState == SEND_PREAMBLE_AWAKE)
        {
            scheduleAt(simTime(), switching_done);
        }
        else if (macState == SEND_PREAMBLE)
        {
            //scheduleAt(simTime(), switching_done);
        }
        else if (macState == SEND_ACK)
        {
            scheduleAt(simTime() + 0.5f * checkInterval, delay_for_ack_within_remote_rx);
        }
        else if (macState == SEND_DATA)
        {
            scheduleAt(simTime(), switching_done);
        }
    }
}

void RIMac::attachSignal(Packet *packet, simtime_t_cref startTime)
{
    simtime_t duration = packet->getBitLength() / bitrate;
    packet->setDuration(duration);
}
/**
 * Change the color of the node for animation purposes.
 */
void RIMac::printState(void)
{
    return;
}

/**
 * Change the color of the node for animation purposes.
 * accepts #FFFFFF format or a color name documented at the omnetpp manual
 */
void RIMac::changeColor(char * color)
{
    if (!animation)
        return;
    cDisplayString& dispStr = getContainingNode(this)->getDisplayString();
    dispStr.setTagArg("t", 2,  color);
    return;
}
/**
 * Change the color of the node for animation purposes.
 */
void RIMac::changeDisplayColor(RIMAC_COLORS color)
{
    if (!animation)
        return;
    cDisplayString& dispStr = getContainingNode(this)->getDisplayString();

    switch (macState) {
        case INIT:
            dispStr.setTagArg("t", 0, "INIT");
            dispStr.setTagArg("t", 2, "orange");
            break;

        case SLEEP:
            dispStr.setTagArg("t", 0, "SLEEP");
            dispStr.setTagArg("t", 2, "#595959");
            break;

        case CCA:
            dispStr.setTagArg("t", 0, "CCA");
            dispStr.setTagArg("t", 2, "#FF0000");
            break;
        case WAIT_REPORT:
            dispStr.setTagArg("t", 0, "WAIT_REPORT");
            dispStr.setTagArg("t", 2, "#FFFF00");
            break;
        case SEND_REPORT:
            dispStr.setTagArg("t", 0, "SEND_REPORT");
            dispStr.setTagArg("t", 2, "#006600");
            break;
        case WAIT_PREAMBLE:
            dispStr.setTagArg("t", 0, "WAIT_PREAMBLE");
            dispStr.setTagArg("t", 2, "#FFD500");
            break;
        case SEND_PREAMBLE:
            dispStr.setTagArg("t", 0, "SEND_PREAMBLE");
            dispStr.setTagArg("t", 2, "#A2FF00");
            break;
        case SEND_BEACON_W:
            dispStr.setTagArg("t", 0, "SEND_BEACON_W");
            dispStr.setTagArg("t", 2, "#7E7027");
            break;
        case SEND_BEACON:
            dispStr.setTagArg("t", 0, "SEND_BEACON");
            dispStr.setTagArg("t", 2, "#00FF13");
            break;
        case WAIT_ACK:
            dispStr.setTagArg("t", 0, "WAIT_ACK");
            dispStr.setTagArg("t", 2, "#7FFFFF");
            break;
        case WAIT_BEACON_TX_END:
            dispStr.setTagArg("t", 0, "TRANSMITTING BEACON");
            dispStr.setTagArg("t", 2, "#00FF13");
            break;
        case WAIT_BACKOFF:
            dispStr.setTagArg("t", 0, "WAIT_BACKOFF");
            dispStr.setTagArg("t", 2, "#FF5500");
            break;
        case WAIT_BEACON:
            dispStr.setTagArg("t", 0, "WAIT_BEACON");
            dispStr.setTagArg("t", 2, "#FFB900");
            break;
        case WAIT_TX_DATA_OVER:
            dispStr.setTagArg("t", 0, "TRANSMITTING DATA");
            dispStr.setTagArg("t", 2, "#001FFF");
            break;

        default:
            dispStr.setTagArg("t", 0, "");
            break;
    }


    //dispStr.setTagArg("i", 0, edited);
}


void RIMac::decapsulate(Packet *packet)
{
    const auto& rimacHeader = packet->popAtFront<RIMacHeader>();
    packet->addTagIfAbsent<MacAddressInd>()->setSrcAddress(rimacHeader->getSrcAddr());
    packet->addTagIfAbsent<InterfaceInd>()->setInterfaceId(interfaceEntry->getInterfaceId());
    auto payloadProtocol = ProtocolGroup::ethertype.getProtocol(rimacHeader->getNetworkProtocol());
    packet->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(payloadProtocol);
    packet->addTagIfAbsent<PacketProtocolTag>()->setProtocol(payloadProtocol);
    EV_DETAIL << " message decapsulated " << endl;
}

void RIMac::encapsulate(Packet *packet)
{
    auto pkt = makeShared<RIMacHeader>();
    pkt->setChunkLength(b(headerLength));

    // copy dest address from the Control Info attached to the network
    // message by the network layer
    auto dest = packet->getTag<MacAddressReq>()->getDestAddress();
    EV_DETAIL << "CInfo removed, mac addr=" << dest << endl;
    pkt->setNetworkProtocol(ProtocolGroup::ethertype.getProtocolNumber(packet->getTag<PacketProtocolTag>()->getProtocol()));
    pkt->setDestAddr(dest);

    //delete the control info
    delete packet->removeControlInfo();

    //set the src address to own mac address (nic module getId())
    pkt->setSrcAddr(interfaceEntry->getMacAddress());

    pkt->setType(RIMAC_REPORT_MSG);
    packet->setKind(RIMAC_REPORT_MSG);

    //encapsulate the network packet
    packet->insertAtFront(pkt);
    packet->getTag<PacketProtocolTag>()->setProtocol(&Protocol::rimac);
    EV_DETAIL << "pkt encapsulated\n";
}



} // namespace inet
