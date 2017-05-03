/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

 /*
  * Note: You can change/add any functions in MP1Node.{h,cpp}
  */

  /**
   * Overloaded Constructor of the MP1Node class
   * You can add new members to the class if you think it
   * is necessary for your logic to work
   */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for (int i = 0; i < 6; i++) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
	this->lastEntry = NULL;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
	if (memberNode->bFailed) {
		return false;
	}
	else {
		return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
	}
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
	Address joinaddr;
	joinaddr = getJoinAddress();

	// Self booting routines
	if (initThisNode(&joinaddr) == -1) {
#ifdef DEBUGLOG
		log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
		exit(1);
	}

	if (!introduceSelfToGroup(&joinaddr)) {
		finishUpThisNode();
#ifdef DEBUGLOG
		log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
		exit(1);
	}

	return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	this->id = id;
	this->port = port;
	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
	// node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
	initMemberListTable(memberNode);

	return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
	static char s[1024];
#endif

	if (0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
		// I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
		log->LOG(&memberNode->addr, "Starting up group...");
#endif
		memberNode->inGroup = true;
	}
	else {
		size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1 + sizeof(int);
		msg = (MessageHdr *)malloc(msgsize * sizeof(char));

		// create JOINREQ message: format of data is {struct Address myaddr}
		msg->msgType = JOINREQ;
		memcpy((char *)(msg + 1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
		memcpy((char *)(msg + 1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
		memcpy((char *)(msg + 1) + 1 + sizeof(memberNode->addr.addr) + sizeof(long), &id, sizeof(int));

#ifdef DEBUGLOG
		sprintf(s, "Trying to join...\n");
		log->LOG(&memberNode->addr, s);
#endif

		// send JOINREQ message to introducer member
		emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

		free(msg);
	}

	return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode() {
	/*
	 * Your code goes here
	 */
	return true;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
	if (memberNode->bFailed) {
		return;
	}

	// Check my messages
	checkMessages();

	// Wait until you're in the group...
	if (!memberNode->inGroup) {
		return;
	}

	// ...then jump in and share your responsibilites!
	nodeLoopOps();

	return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
	void *ptr;
	int size;

	// Pop waiting messages from memberNode's mp1q
	while (!memberNode->mp1q.empty()) {
		ptr = memberNode->mp1q.front().elt;
		size = memberNode->mp1q.front().size;
		memberNode->mp1q.pop();
		recvCallBack((void *)memberNode, (char *)ptr, size);
	}
	return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size) {
	/*
	 * Your code goes here
	 */

	 //extract message type
	MessageHdr* msg = (MessageHdr *)malloc(MSGTYPESIZE * sizeof(char));
	memcpy(&msg->msgType, data, MSGTYPESIZE);
	switch (msg->msgType) {

	case(JOINREQ):
		joinHandler(data, size);
		break;

	case(JOINREP):
		//get ack from booster, online
		joinrepHandler();
		break;
	case(PING):
		break;
	case(ACK):
		break;
	case(SUBPING):
		break;
	case(SUBPINGREQ):
		break;
	case(SUBPINGREP):
		break;
	case(SUBPINGACK):
		break;

	case(DUMMYLASTMSGTYPE):
	default:
		//Unknown msgType
#ifdef DEBUGLOG
		static char s[1024];
		sprintf(s, "error: Unknown msgType %d\n", msg->msgType);
		log->LOG(&memberNode->addr, s);
#endif
		return false;
	}

	free(msg);
	return true;
}

/**
* FUNCTION NAME: joinHandler
*
* DESCRIPTION: Handle a new node join
*/
bool MP1Node::joinHandler(char *data, int size) {
#ifdef DEBUGLOG
	static char s[1024];
#endif
	MemberListEntry* memberListEntry;
	int id;
	long heartbeat;
	MessageHdr* msg;

	//get address
	Address* addr = (Address *)malloc(ADDRSIZE * sizeof(char));
	memcpy(&addr->addr, data + MSGTYPESIZE, ADDRARYSIZE);

	if (isNullAddress(addr)) {
#ifdef DEBUGLOG
		sprintf(s, "error: null message address.\n");
		log->LOG(&memberNode->addr, s);
#endif
		free(addr);
		return false;
	}

	//get heartbeat
	memcpy(&heartbeat, data + MSGTYPESIZE + 1 + ADDRSIZE, sizeof(long));

	//get id
	memcpy(&id, data + MSGTYPESIZE + 1 + ADDRSIZE + sizeof(long), sizeof(int));

#ifdef DEBUGLOG
	sprintf(s, "Node id=%d was added to MemberListEntry.\n", id);
	log->LOG(&memberNode->addr, s);
#endif

	memberListEntry = new MemberListEntry(id, addr->addr[4], heartbeat, par->getcurrtime());
	memberNode->memberList.push_back(*memberListEntry);
	memberNode->nnb++;//increment number of neighbors

#ifdef DEBUGLOG
	sprintf(s, "Node id=%d was added to group.\n", id);
	log->LOG(&memberNode->addr, s);
#endif
	log->logNodeAdd(&memberNode->addr, addr);

	//sen JOINREP back
	size_t msgsize = MSGTYPESIZE + 1;
	msg = (MessageHdr *)malloc(msgsize * sizeof(char));

	// create JOINREP message: format of data is {struct Address myaddr}
	msg->msgType = JOINREP;

	// send JOINREP message to put member online
	emulNet->ENsend(&memberNode->addr, addr, (char *)msg, msgsize);

	free(msg);
	free(addr);
	return true;
}

/**
* FUNCTION NAME: joinrepHandler
*
* DESCRIPTION: Put this node online and initiate counter and member list
*/
void MP1Node::joinrepHandler() {
	memberNode->inGroup = true;
	initCounter();
	memberNode->myPos = memberNode->memberList.end();
	//check the member list and put this node in the list
	auto list = memberNode->memberList.begin();
	for (; list != memberNode->memberList.end(); list++) {
		if (list->id == id) {
			list->heartbeat = memberNode->heartbeat;
			list->timestamp = par->globaltime;
			memberNode->myPos = list;
			break;
		}
	}
	if (memberNode->myPos == memberNode->memberList.end()) {
		MemberListEntry entry = MemberListEntry(id, memberNode->addr.addr[4], memberNode->heartbeat, par->getcurrtime());
		memberNode->memberList.push_back(entry);
		memberNode->myPos = memberNode->memberList.end()-1;
	}

#ifdef DEBUGLOG
	static char s[1024];
	sprintf(s, "Node id=%d is online with heartbeat=%ld, timestamp=%ld.\n", this->id, memberNode->myPos->heartbeat, memberNode->myPos->timestamp);
	log->LOG(&memberNode->addr, s);
#endif
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
#ifdef DEBUGLOG
	static char s[1024];
#endif

	//update new heartbeat
	updateStatus();

#ifdef DEBUGLOG
	sprintf(s, "My heartbeat goes to %ld.\n", memberNode->heartbeat);
	log->LOG(&memberNode->addr, s);
#endif

	//counter goes down
	countDownCounter();
	checkCounter();

	return;
}

/**
* FUNCTION NAME: checkCounter
*
* DESCRIPTION: check the counter to decide the next step
*/
void MP1Node::checkCounter() {
	if (memberNode->pingCounter == 0) {
		//remove last
		if (lastEntry != NULL) {
			logRemoveEntry();
			lastEntry->timestamp = -1;//set as failed node
			lastEntry = NULL;
		}

		//prepare ping message and send
		sendPing();
		//reset pingcounter and timeout counter
		initCounter();
	}

	if (memberNode->timeOutCounter == 0) {
		//prepare subping message
		sendSubping();

		//set timeoutcounter to -1
		memberNode->timeOutCounter = -1;//set to nagative to avoid counter 0 again
	}
}

/**
* FUNCTION NAME: sendPing
*
* DESCRIPTION: prepare ping message with message type, address, and piggyback infos then send
*/
void MP1Node::sendPing() {

#ifdef DEBUGLOG
	static char s[1024];
#endif

	//check if aviliable neighbor to send ping
	if (memberNode->nnb > 0) {
		size_t msgsize = sizeof(MessageHdr) + sizeof(Address) + 1 + sizeof(size_t) + sizeof(MemberListEntry) * memberNode->memberList.size();
		MessageHdr* msg = (MessageHdr *)malloc(msgsize * sizeof(char));

		msg->msgType = PING;
		memcpy((char *)(msg + 1), &memberNode->addr.addr, ADDRARYSIZE);

		//fill payload
		fillPiggyback((char *)msg, (size_t)1 + ADDRARYSIZE + 1);

		//select a random live neighbor
		Address toAddr = getRandomNeighbor();

#ifdef DEBUGLOG
		sprintf(s, "Ping send to %s.\n", toAddr.getAddress().c_str());
		log->LOG(&memberNode->addr, s);
#endif

		// send PING message to detect member
		emulNet->ENsend(&memberNode->addr, &toAddr, (char *)msg, msgsize);

		free(msg);
	}
	else {
#ifdef DEBUGLOG
		sprintf(s, "No neighbor aviliable.\n");
		log->LOG(&memberNode->addr, s);
#endif
	}

}


/**
* FUNCTION NAME: sendSubping
*
* DESCRIPTION: prepare ping message with message type, from address, to address and piggyback infos then send
*/
void MP1Node::sendSubping() {

#ifdef DEBUGLOG
	static char s[1024];
#endif

	//check if more aviliable neighbor to send ping
	if (memberNode->nnb > 1) {
		//prepare message
		size_t msgsize = sizeof(MessageHdr) + 2 * (sizeof(Address) + 1) + sizeof(size_t) + sizeof(MemberListEntry) * memberNode->memberList.size();
		MessageHdr* msg = (MessageHdr *)malloc(msgsize * sizeof(char));

		msg->msgType = SUBPING;
		memcpy((char *)(msg + 1), &memberNode->addr.addr, ADDRARYSIZE);
		Address destination = getListEntryAddr(lastEntry);
		memcpy((char *)(msg + 1 + ADDRARYSIZE + 1), &destination.addr, ADDRARYSIZE);//destination address

		//fill payload
		fillPiggyback((char *)msg, (size_t) 1 + 2*(ADDRARYSIZE + 1));

#ifdef DEBUGLOG
		sprintf(s, "SUBPING to detect %s is ready.\n", getListEntryAddr(lastEntry).getAddress().c_str());
		log->LOG(&memberNode->addr, s);
#endif

		vector<MemberListEntry>::iterator list;
		//select live neighbors to send subping
		for (list = memberNode->memberList.begin(); list != memberNode->memberList.end(); list++) {
			if ( &(*list) != lastEntry && list->timestamp >= 0) {//valid and alive
				Address toAddr = getListEntryAddr(&(*list));//get the address of neighbor
#ifdef DEBUGLOG
				sprintf(s, "Send SUBPING to %s.\n", toAddr.getAddress().c_str());
				log->LOG(&memberNode->addr, s);
#endif
				// send SUBPING message to detect member
				emulNet->ENsend(&memberNode->addr, &toAddr, (char *)msg, msgsize);
			}
		}
		free(msg);
	}
	else {
#ifdef DEBUGLOG
		sprintf(s, "No neighbor aviliable.\n");
		log->LOG(&memberNode->addr, s);
#endif
	}
}

/**
* FUNCTION NAME: fillPiggyback
*
* DESCRIPTION: Take the pointer of message and offset to fill all memberlistentry
*/
void MP1Node::fillPiggyback(char *msg, size_t offset) {
	size_t size = memberNode->memberList.size();
	memcpy(msg + offset, &size, sizeof(size_t));
	auto list = memberNode->memberList.begin();
	int i;
	for (i = 0; list != memberNode->memberList.end(); list++, i++) {
		memcpy(msg + offset + sizeof(size_t) + i * sizeof(MemberListEntry), &(*list), sizeof(MemberListEntry));
	}
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
	Address joinaddr;

	memset(&joinaddr, 0, sizeof(Address));
	*(int *)(&joinaddr.addr) = 1;
	*(short *)(&joinaddr.addr[4]) = 0;

	return joinaddr;
}


void MP1Node::logRemoveEntry() {

	//build addr of removed node
	Address neighborAddr;
	memset(&neighborAddr, 0, sizeof(Address));
	*(int *)(&neighborAddr.addr) = lastEntry->id;
	*(short *)(&neighborAddr.addr[4]) = lastEntry->port;

#ifdef DEBUGLOG
	static char s[1024];
	sprintf(s, "Node %s was removed from node %s at %d.\n", neighborAddr.getAddress().c_str(), memberNode->addr.getAddress().c_str(), par->globaltime);
	log->LOG(&memberNode->addr, s);
#endif

	//log the remove node
	log->logNodeRemove(&memberNode->addr, &neighborAddr);
}

/**
* FUNCTION NAME: getRandomNeighbor
*
* DESCRIPTION: Returns the Address of the selected random living neighbor
*/
Address MP1Node::getRandomNeighbor() {
	int i = rand() % memberNode->memberList.size();

	while (memberNode->memberList[i].timestamp == -1) {
		i = rand() % memberNode->memberList.size();
	}
	lastEntry = &memberNode->memberList[i];


	return getListEntryAddr(lastEntry);
}


/**
* FUNCTION NAME: getListEntryAddr
*
* DESCRIPTION: Returns the Address of the coordinator
*/
Address MP1Node::getListEntryAddr(MemberListEntry* entry) {
	Address neighborAddr;
	memset(&neighborAddr, 0, sizeof(Address));
	*(int *)(&neighborAddr.addr) = entry->id;
	*(short *)(&neighborAddr.addr[4]) = entry->port;
	return neighborAddr;
}


/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
	printf("%d.%d.%d.%d:%d \n", addr->addr[0], addr->addr[1], addr->addr[2],
		addr->addr[3], *(short*)&addr->addr[4]);
}

/**
* FUNCTION NAME: initCounter
*
* DESCRIPTION: reset the counter and time out
*/
void MP1Node::initCounter() {
	memberNode->pingCounter = TREMOVE;
	memberNode->timeOutCounter = TFAIL;
}

/**
* FUNCTION NAME: updateStatus
*
* DESCRIPTION: increment heartbeat
*/
void MP1Node::updateStatus() {
	memberNode->heartbeat++;
	memberNode->myPos->heartbeat = memberNode->heartbeat;
}

/**
* FUNCTION NAME: countDownCounter
*
* DESCRIPTION: decrement every counter
*/
void MP1Node::countDownCounter() {
	memberNode->pingCounter--;
	memberNode->timeOutCounter--;
}

