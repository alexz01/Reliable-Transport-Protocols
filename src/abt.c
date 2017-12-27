#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "../include/simulator.h"

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SIX ROUTINES *********/

#define BUFFER_SIZE 10000
#define TIMER 20.0f
#define MSG_SIZE 20
int seqnum_ctr;        // store the next sequence number
int B_waiting_for_seq; // B waiting for a packet seqno
int current_pkt;       // store head of buffer
int buffer_index;      // store the end tail of buffer

struct buffer          // buffer implemented as a circular queue
{
	struct pkt *packet;
	int active;
};

struct buffer buf[BUFFER_SIZE];



/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	int i;
	printf("\n");

	//make a packet
	struct pkt *new_pkt = malloc(sizeof(struct pkt));
	
	strncpy(new_pkt->payload,message.data, MSG_SIZE);
	
	new_pkt->seqnum = seqnum_ctr;
	new_pkt->acknum = seqnum_ctr;

	new_pkt->checksum = new_pkt->seqnum;
	new_pkt->checksum += new_pkt->acknum;
	for(i = 0; i< 20; i++)
	{
		new_pkt->checksum += (int)message.data[i];
//		printf("A's checksum: %d\n",new_pkt->checksum);
	}
	//printf("checksum: %d\n",new_pkt->checksum);

	if(buffer_index == current_pkt ) 
	{	
		if(buf[current_pkt].active==1) // the buffer is full as buffer index has rounded over and came behind the current packet in circular queue
		{
			printf("Buffer is full\n");
			--buffer_index;
			exit(1);
		}	
		else // buffer is completely empty, store packet in buffer and transmit immediately
		{
			
			
			buf[buffer_index].packet = new_pkt;
			buf[buffer_index].active = 1;
			printf("%d %d %d sending packet %s\n",seqnum_ctr, current_pkt, buffer_index, buf[buffer_index].packet->payload);
			tolayer3(0,*new_pkt);
			starttimer(0,TIMER);
			
		}
	}
	else 
	{
		buf[buffer_index].packet = new_pkt;
		buf[buffer_index].active = 1;
		printf("%d %d %d buffering packet %s\n",seqnum_ctr, current_pkt, buffer_index, buf[buffer_index].packet->payload);
	}
	seqnum_ctr = (++seqnum_ctr)%2;
	buffer_index = (++buffer_index)%BUFFER_SIZE;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	printf("inside A_input: ");
	
	printf("received from 3: ack:%d seq %d %s curr_seq: %d chk: %d ",packet.acknum,packet.seqnum, packet.payload,current_pkt,packet.checksum);
	int i = 0, checksum = packet.acknum;
	checksum += packet.seqnum;
	for(i = 0; i < 3; i++)
	{
		checksum += packet.payload[i];
		printf("A's chk: %d %d %c\n",checksum, packet.payload[i],packet.payload[i] );
	}
	printf("calc chk: %d\n",checksum);
	if(packet.checksum == checksum)
	{
		//waiting for the ack of packet.acknum and send packet only when ack is the expected one
		if(packet.acknum == buf[current_pkt].packet->seqnum)
		{
			//ack of current packet is received send the next packet
			stoptimer(0);
			current_pkt++;
			printf("buf[current_pkt].active = %d\n",buf[current_pkt].active);
			if(buf[current_pkt].active==1)
			{
				printf("%d sending from buffer: %d %s %d , ",current_pkt,buffer_index,buf[current_pkt].packet->payload,strlen(buf[current_pkt].packet->payload));
				tolayer3(0,*(buf[current_pkt].packet));
				starttimer(0,TIMER);
			}
		}
	}
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	printf("timeout occured for %d, seq=%d %s\n", current_pkt, buf[current_pkt].packet->seqnum,buf[current_pkt].packet->payload);
	tolayer3(0,*(buf[current_pkt].packet));
	starttimer(0,TIMER);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	int i;
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		buf->active = 0;
	}
	seqnum_ctr = 0;
	current_pkt = 0;
	buffer_index = 0;

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	printf("inside b_input : seq:%d %d %s len:%d chk:%d , ",packet.seqnum,packet.acknum,packet.payload,strlen(packet.payload),packet.checksum);
	int i = 0;
	int checksum = packet.seqnum;
	//printf("calc chck: %d\n",checksum);
	checksum += packet.acknum;
	//printf("calc chck: %d\n",checksum);
	for(i = 0; i < 20; i++)
	{
		checksum += packet.payload[i];
		//printf("calc chck: %d\n",checksum);
	}
	printf("calc chk: %d \n",checksum);
	
	if(packet.checksum == checksum)
	{
		if(packet.seqnum == B_waiting_for_seq)
		{
			printf("B sending to layer5\n");
			tolayer5( 1, packet.payload);
			memset(&packet.payload,0,strlen(packet.payload));
			strcpy(packet.payload,"ACK\0");
			packet.acknum = packet.seqnum;
			packet.checksum = 0;
			for(i = 0; i <3; i++)
			{
				packet.checksum += (int)packet.payload[i];
			}
			packet.checksum += packet.acknum;
			printf("exiting b's input\n");
			B_waiting_for_seq = (++B_waiting_for_seq)%2;
			tolayer3 (1, packet);
			printf("exited b's input\n");
		}
		else
		{
			
			struct pkt *pak = malloc (sizeof(struct pkt));
			pak->acknum = packet.seqnum;
			pak->seqnum = packet.seqnum;
			memset(&pak->payload,0,strlen(pak->payload));
			strcpy(pak->payload, "ACK\0");
			int i = 0; 
			pak->checksum = packet.acknum;
			pak->checksum += packet.seqnum;
			for(i = 0; i < 3; i++)
			{
				pak->checksum  += pak->payload[i];
				printf("B's chk: %d %d %c\n",pak->checksum,pak->payload[i],pak->payload[i] );
			}

			tolayer3(1,*pak);
		}
	}
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	B_waiting_for_seq =0;
}