//
// Copyright (C) 2017 Jan Peter Drees
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

import inet.common.INETDefs;
import inet.linklayer.base.MacHeaderBase;

namespace inet;

// @brief Types of messages (self messages and packets) the node can process
enum RIMacTypes {
    // packet types
    RIMAC_PREAMBLE = 191;
    RIMAC_BEACON = 216;
    RIMAC_DATA = 192;
    RIMAC_ACK = 193;
    // self message types
    RIMAC_RESEND_DATA = 194;
    RIMAC_ACK_TIMEOUT = 195;
    RIMAC_START_RIMAC = 196;
    RIMAC_WAKE_UP = 197;
    RIMAC_SEND_ACK = 198;
    RIMAC_CCA_TIMEOUT = 199;
    RIMAC_ACK_TX_OVER = 200;
    RIMAC_SEND_PREAMBLE = 201;
    RIMAC_STOP_PREAMBLES = 202;
    RIMAC_DATA_TX_OVER = 203;
    RIMAC_DATA_TIMEOUT = 204;
    SWITCH_PREAMBLE_PHASE = 205;
    DELAY_FOR_ACK_WITHIN_REMOTE_RX = 206;
    RIMAC_SWITCHING_FINISHED = 207;
    RIMAC_STOP_RXMODE = 208;
    RIMAC_SEND_REPORT = 209;
    RIMAC_REPORT_TIMEOUT = 210;
    RIMAC_WAIT_TIMEOUT = 211;
    RIMAC_PREAMBLE_TIMEOUT = 212;
    RIMAC_REPORT_MSG	= 213;
    RIMAC_CARRIERSENSE = 214;
    RIMAC_START_CCA = 215;
    //RIMAC_BEACON = 216;
    RIMAC_COLLISION_ENDED = 217;
    RIMAC_BACKOFF_TO = 218;
    RIMAC_RCV_STARTED = 219;
};

//
// Frame for ~RIMac. Frame size is configurable
// in the MAC layer. 
//
class RIMacHeader extends MacHeaderBase
{
    RIMacTypes type;
    long sequenceId; // Sequence Number to detect duplicate messages
    int  windowSize; // Backoff window to generate when collision has been detected
}

