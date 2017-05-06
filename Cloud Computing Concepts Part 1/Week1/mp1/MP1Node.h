/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"
#include <map>
#include <algorithm>

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5
#define MSGTYPESIZE sizeof(MessageHdr)
#define ADDRSIZE sizeof(Address)
#define ADDRARYSIZE sizeof(memberNode->addr.addr)

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,
    JOINREP,
	PING,
	ACK,
	SUBPING,
	SUBPINGREQ,
	SUBPINGREP,
	SUBPINGACK,
    DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
}MessageHdr;

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	MemberListEntry *lastEntry;
	int id;
	int port;
	char NULLADDR[6];

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	bool joinHandler(char *data, int size);
	void joinrepHandler();

	void pingHandler(char *data);
	void ackHandler(char *data, int size);
	void subpingHandler(char *data, size_t size);
	void subpingreqHandler(char *data, size_t size);
	void subpingrepHandler(char *data, size_t size);
	void subpingackHandler(char *data, size_t size);

	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
	void initCounter();
	void updateStatus();
	void countDownCounter();
	void checkCounter();
	void fillPiggyback(char *msg, unsigned int offset);
	void processPiggyback(char *msg, unsigned int offset);

	void updateMemberList(MemberListEntry* entry);

	void sendPing();
	void sendSubping();
	void sendACK(Address* addr);
	void sendSubpingreq(Address* srcaddr, Address* destaddr);
	void sendSubpingrep(Address* srcaddr, Address* midaddr);
	void sendSubpingack(Address* srcaddr, Address* destaddr);

	Address getRandomNeighbor();
	void logRemoveEntry();
	void logRemoveEntry(MemberListEntry *entryToRemove);
	Address getListEntryAddr(MemberListEntry* entry);

	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
