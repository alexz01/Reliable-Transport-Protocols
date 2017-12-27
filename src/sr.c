#include "../include/simulator.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>


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

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

#define BUFFER_SIZE 10000
#define TIMER 20.0f
#define DEFAULT_TIMER 1.0f

#define MSG_SIZE 20
int snd_base;
int rcv_base;
int base;
int window;
int next_seq_num;        // store the next sequence number
int send_next;			//next packet to be sent
int B_waiting_for_seq; // B waiting for a packet seqno
int current_pkt;       // store head of buffer
int buffer_index;      // store the end tail of buffer

struct buffer          // buffer implemented as a circular queue
{
	struct pkt *packet;
	int active;
};

struct timers 
{
	int seqnum;
	int remaining;
	int active;
};

struct buffer snd_buffer[BUFFER_SIZE];
struct buffer rcv_buffer[BUFFER_SIZE];
struct timers all_timers[BUFFER_SIZE];


/* called from layer 5, passed the data to be sent to other side */
void A_output(message)
  struct msg message;
{
	int i;
	printf("\n");

	//make a packet
	struct pkt *new_pkt = malloc(sizeof(struct pkt));
	
	strncpy(new_pkt->payload,message.data, MSG_SIZE);
	
	new_pkt->seqnum = next_seq_num;
	new_pkt->acknum = next_seq_num;

	new_pkt->checksum = new_pkt->seqnum;
	new_pkt->checksum += new_pkt->acknum;
	for(i = 0; i< 20; i++)
	{
		new_pkt->checksum += (int)message.data[i];
//		printf("A's checksum: %d\n",new_pkt->checksum);
	}

	
	if(next_seq_num < snd_base + window )
	{	//buffer and send the packet
		snd_buffer[buffer_index].packet = new_pkt;
		snd_buffer[buffer_index].active = 1;
		printf("[A]B%d S%d sending packet %s\n",snd_base, next_seq_num,  snd_buffer[buffer_index].packet->payload);
		//add timer
		all_timers[next_seq_num].seqnum = next_seq_num;
		all_timers[next_seq_num].remaining = TIMER;		
		all_timers[next_seq_num].active = 1;		
		
		send_next++; 
		tolayer3(0,*new_pkt);
		
		if(next_seq_num== 0)   //the first message sent
			starttimer(0, DEFAULT_TIMER);
		
	}
	else
	{	//buffer packet as it's out of window
		snd_buffer[buffer_index].packet = new_pkt;
		snd_buffer[buffer_index].active = 1;
		printf("[A]B%d S%d buffering packet %s\n",snd_base, next_seq_num, snd_buffer[buffer_index].packet->payload);
		
	}
	
	next_seq_num = (++next_seq_num)%BUFFER_SIZE;
	buffer_index = (++buffer_index)%BUFFER_SIZE;

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(packet)
  struct pkt packet;
{
	int prev_last_seq;
	//printf("[A]inside A_input: ");
	
	printf("[A]%d-%d received from 3: %d %s\n",snd_base, snd_base+window,packet.acknum,packet.payload);
	int i = 0, checksum = packet.acknum;
	for(i = 0; i < 20; i++)
	{
		checksum += packet.payload[i];
	}
	if(packet.checksum == checksum)
	{
		//waiting for the ack of packet.acknum and send packet only when ack is the expected one
		if(packet.acknum >= snd_base && packet.acknum < snd_base + window)
		{
			//ack of current packet is received send the next packet
			if(snd_buffer[packet.acknum].active ==1)
			{
				snd_buffer[packet.acknum].active = 0;
				all_timers[packet.acknum].active = 0;
				all_timers[packet.acknum].remaining = -1;
				free(snd_buffer[packet.acknum].packet);
				if(packet.acknum==snd_base)
				{
					prev_last_seq = snd_base + window;
					int new_base = snd_base;
					for(i = snd_base ; i <send_next; i++)
					{
						if(snd_buffer[i].active == 0)
						{
							new_base = i;
							
						}
					}
					snd_base = new_base+1;
					for(current_pkt = prev_last_seq + 1; current_pkt < snd_base + window ; current_pkt++)
						if(snd_buffer[current_pkt].active==1)
						{
							printf("[A]%d-%d sending from buffer: %d base:%d \n",snd_base,snd_base+window,current_pkt);
							send_next++;
							all_timers[current_pkt].seqnum = current_pkt;
							all_timers[current_pkt].remaining = TIMER;
							all_timers[current_pkt].active = 1;						
							tolayer3(0,*(snd_buffer[current_pkt].packet));
							starttimer(0,DEFAULT_TIMER);
							
						}
					
					
				}
			}
/*			base = packet.acknum + 1;
			if(base == next_seq_num)
				stoptimer(0);
			else
			{
				stoptimer(0);
				starttimer(0,TIMER);
			}
			for(current_pkt = base; current_pkt < base + window ; current_pkt++)
				if(snd_buffer[current_pkt].active==1)
				{
					printf("%d sending from buffer: %d %s %d , ",current_pkt,base,snd_buffer[current_pkt].packet->payload,strlen(snd_buffer[current_pkt].packet->payload));
					tolayer3(0,*(snd_buffer[current_pkt].packet));
					starttimer(0,TIMER);
				}
				*/
		}
	}

}

/* called when A's timer goes off */
void A_timerinterrupt()
{
	int i=0;
	
	for(i = snd_base; i < snd_base + window; i++)
	{
		if(all_timers[i].active)
		{
			
			//printf("seq:%d t:%d, ",all_timers[i].seqnum,all_timers[i].remaining);
			all_timers[i].remaining--;
		}
		
	}
	for(i = snd_base; i < snd_base + window; i++)
	{
		if(all_timers[i].remaining == 0 && all_timers[i].active == 1 )
		{
			//reset timer
			all_timers[i].remaining = TIMER;
			//retransmit
			printf("[A]%d-%d Retransmitting seq: %d\n",snd_base,snd_base+window, all_timers[i].seqnum);
			tolayer3(0,*(snd_buffer[i].packet));
		}
	}
	
	starttimer(0,DEFAULT_TIMER);

}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
	int i;
	snd_base = 0;
	for(i = 0; i < BUFFER_SIZE; i++)
	{
		snd_buffer[i].active = 0;
		all_timers[i].active = 0;
		all_timers[i].remaining = -1;
	}
	window = getwinsize();
	next_seq_num = 0;
	//current_pkt = 0;
	buffer_index = 0;
	send_next = 0;
		

}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(packet)
  struct pkt packet;
{
	printf("[B]%d-%d received seq:%d \n",rcv_base,rcv_base+window,packet.seqnum);
	int i;
	if(packet.seqnum >= rcv_base && packet.seqnum < rcv_base + window)
	{
		
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
		//printf("calc chk: %d \n",checksum);
		
		if(packet.checksum == checksum)
		{
			//buffer packet
			struct pkt *new_rcv_pkt = malloc(sizeof(struct pkt));
			new_rcv_pkt->seqnum = packet.seqnum;
			new_rcv_pkt->acknum = packet.acknum;
			strncpy(new_rcv_pkt->payload, packet.payload, MSG_SIZE);
			
			rcv_buffer[new_rcv_pkt->seqnum].packet = new_rcv_pkt;
			rcv_buffer[new_rcv_pkt->seqnum].active = 1;
			
			//sent ACK
			memset(&packet.payload,0,sizeof(packet.payload));
			strcpy(packet.payload,"ACK\0");
			packet.acknum = packet.seqnum;
			packet.checksum = 0;
			for(i = 0; i <20; i++)
			{
				packet.checksum += (int)packet.payload[i];
			}
			packet.checksum += packet.acknum;
			//printf("exiting b's input\n");
			//B_waiting_for_seq = (++B_waiting_for_seq);
			printf("[B] Sending ACK for seq %d\n", packet.acknum);
			tolayer3 (1, packet);
			//printf("exited b's input\n");
			
			//check if packet is base 
			//if base then send from base to all the next inorder packets ie. till a hole in buffer
			if(new_rcv_pkt->seqnum == rcv_base)
			{
				printf("[B] Base received of seq: %d\n",rcv_base);
				for(i = rcv_base; i < rcv_base + window; i++)
				{
					if( ! rcv_buffer[i].active)
					{
						rcv_base = i;
						break;
					}
					printf("[B] sending to layer5 : %d %s\n", i, rcv_buffer[i].packet->payload);
					tolayer5( 1, rcv_buffer[i].packet->payload);
					//free receiver buffer
					//free(rcv_buffer[i].packet);
					rcv_buffer[i].active = 0;
				}
			}			
		}
	}
	else if(packet.seqnum >= rcv_base - window && packet.seqnum < rcv_base )
	{
		//duplicate received do nothing
		printf("[B] Retransmitting ACK%d\n",packet.seqnum);
		struct pkt *pak = malloc (sizeof(struct pkt));
		pak->acknum = packet.seqnum;
		strncpy(pak->payload, "ACK\0", MSG_SIZE);
		pak->checksum = 0;
		for(i = 0; i <20; i++)
		{
			pak->checksum += (int)pak->payload[i];
		}
		pak->checksum += pak->acknum;

		tolayer3(1,*pak);
		free(pak);
	}
	else
	{
		//ignore the packet
		printf("[B] Ignoring packet seq:%d\n",packet.seqnum);
	}


}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
	rcv_base =0;
}
