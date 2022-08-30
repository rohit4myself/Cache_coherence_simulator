
#include<iostream>
#include<bits/stdc++.h>
using namespace std;
#define ull unsigned long long
#include <fstream>

ull msg_putx[8] = {0};

unordered_map<ull,int>mht[8];//array of miss handling table for each L1 cache.
//Class to simulate the state
class DirectoryClass{
public:
  unordered_map<ull,int> state; // Hashmap to store the block's state: -1 for I state , 0 for S state, and 1 for M state.
  unordered_map<ull,list<int>> sharers; // Hashmap to store the pids of the processors sharing a block.
  unordered_map<ull,int> modified; // Hashmap to store the pids of the processor having the block in M state.
  unordered_map<ull,int> pstate; //Hashmap to set the pending state.

  // Method to return the state(M,S,I) of the block.
  int checkstate(ull blocknumber){
    if(state.find(blocknumber) == state.end())
      state[blocknumber] = -1; // i.e. block is in I state
    return state[blocknumber];
  }

  // Method to invalidate a block from all the sharers.
  void invalidateblock(ull blocknumber){
    state[blocknumber] = -1;
    sharers[blocknumber].clear();
  }

  // Method to add block in S state and append the pid inside the sharers hashmap.
  void addsharers(ull blocknumber,int pid){
    state[blocknumber] = 0;
    sharers[blocknumber].push_back(pid);
  }
  //Method to remove a block from the S state and if no one have this block change the state of the block to I state.
  void removesharer(ull blocknumber,int pid){
    sharers[blocknumber].remove(pid);
    // If no one have this block any more changing its state state to I.
    if(sharers[blocknumber].size() == 0){
       if(state[blocknumber]==0)
          state[blocknumber] = -1;
    }
  }
  //Method to return the vector containing the pids of processors sharing a block in S state.
  list<int> get_sharers_pid(ull blocknumber){
    return sharers[blocknumber];
  }
  // Method to add a block in M state and storing the processor's pid in modified hashmap.
  void addmodified(ull blocknumber,int pid){
    state[blocknumber] = 1;
    modified[blocknumber] = pid;
  }
  //Method to retrun the pid of the processor which is having the block in M state.
  int get_modified_pid(ull blocknumber){
    return modified[blocknumber];
  }
  //Method to set the pending state bit.
  void set_pstate(ull blocknumber){
    pstate[blocknumber] = 1;
  }
  //Method to reset the P state bit.
  void reset_pstate(ull blocknumber){
    pstate[blocknumber] = 0;
  }
  //Method to get the P state bit value.
  int get_pstate(ull blocknumber){
    if(pstate.find(blocknumber) == pstate.end()){
      pstate[blocknumber] = 0;
    }
    return pstate[blocknumber];
  }
};

//Class to Implement LRU ---------------------------------------------------------------------------------------------
class Lru
{     list <int> lines1;
      list <int> lines2;
     unordered_map<int, list<int>> sets1[8];
     unordered_map<int, list<int>> sets2;
     public :
       void init();
       void lru1Update(int pid, int set,int way);
       void lru2Update(int,int set,int way);
       int lru1GetLine(int,int set);
       int lru2GetLine(int, int set);
       void lru1Invalidate(int, int set,int way);

};
//method for initializing LRU for L1  and L2 caches
void Lru :: init()
{
  for(int pid =0; pid<8; pid++){
    for(int i = 0;i<64;i++){
      for(int j=0;j<8;j++){
           lines1.push_back(j);
        }
      sets1[pid].insert(make_pair(i,lines1));
      lines1.clear();
    }
  }

    for(int i = 0;i<4096;i++){
      for(int j=0;j<16;j++){
           lines2.push_back(j);
        }
      sets2.insert(make_pair(i,lines2));
      lines2.clear();
    }

}
//method for updating L1 cache on a hit
void Lru::lru1Update(int pid,int set,int way){
     sets1[pid].at(set).remove(way);
     sets1[pid].at(set).push_front(way);
}
// method for updating L2 cache on a hit
void Lru::lru2Update(int bankid,int set,int way){
     sets2.at(set).remove(way);
     sets2.at(set).push_front(way);
}
//method for returning the last line from the linklist
int Lru::lru1GetLine(int pid,int set){
   int way =sets1[pid].at(set).back();
   sets1[pid].at(set).pop_back();
   sets1[pid].at(set).push_front(way);
   return way;
}
//method for updating L2 cache on a miss
int Lru::lru2GetLine(int bankid,int set)
{  int way=sets2.at(set).back();
   sets2.at(set).pop_back();
   sets2.at(set).push_front(way);
   return way;
}
//method for Invalidating from L1 (inclusive policy)
void Lru::lru1Invalidate(int pid,int set,int way)
{
   sets1[pid].at(set).remove(way);
   sets1[pid].at(set).push_back(way);
}
// End of LRU -----------------------------------------------------------------------------------------------------------

//structure for nack msg
struct msg_nack{
  ull memory_addr;
  int timer;
  int message;
  int message2;
};

//structure for message that will enter in l2 queue and l1_input queue
struct reference{
  ull memory_add;
  int pid; // pid of processor requesting the block.
  int message;
  int message2;
  int sharers; //represent number of sharers for Putx response message
/* Possible values of message:
  0:REPL_S, 1:GET; 2:GETX; 3:UPGRADE; 4:SHARING_WB; 5:XFER; 6:INVALIDATION; 7:PUT; 8: INVACK; 9:NACK; 10:XFER_INV, 11:PUTX
*/
};

// structure for memory reference
struct trace_request{
  ull memory_add;
  int load_store; //1:Load Request; 0:Store Request
  ull count ; //count of the machine access.
  int tid = -1; // thread id of the machine access.
};

//Function Declarations
int get_bankid(ull);
void p(int, ull);
void px(int,ull);
void add_putx_message(int , ull );
void add_put_message(int , ull );

//Global Variables start------------------------------------------------------------------------------------
DirectoryClass directory;// Global object of class DirectoryClass.
queue <reference> l2bank[8]; //array of queues. Each queue is for a l2 bank.
queue <reference>  l1_input[8]; //array of queues. Each queue is for a l1 private cache. It receives coherence requests.
queue <trace_request> l1_trace[8];//array of queues. Each queue is for a l1 private cache. It receives traces memory address.
vector <msg_nack> nack_buffer[8];// array of vectors of type msg_nack. Each vector is for a nack buffer for each private cache


ull cycle = 0;
ull l1_accesses[8], l1_hits[8], l1_misses[8],l1_upgrade[8],l1_read_hits[8],l1_write_hits[8],l1_read[8],l1_write[8];
ull l1_read_miss[8], l1_write_miss[8];
ull l1cache[8][64][8], l2cache[4096][16]; // Defining L1 and L2 caches
ull l2accesses =0;
ull l2hits=0;
ull l2misses = 0;
Lru lru; //Object of Lru class.
ull l1messagecounter[8][15];
ull l2messagecounter[12];
ull total_count; // variable to maintain total no. of global count
//Global Variables end -------------------------------------------------------------------------------------

// function for sending nack
void send_nack(reference head){

     reference rqst;
     rqst.memory_add=head.memory_add;
     rqst.pid = head.pid;
     rqst.message = 9;          // 9 is NACK number for message
     rqst.message2 =head.message;
     l1_input[rqst.pid].push(rqst); // pushing NACK message in the input/incoming queue of L1 cache
}
// function to check whether a block can be serviced from nack or not
int l1_check_nack(int pid){
  int check=0;
  if(nack_buffer[pid].empty()) {// if nack buffer is empty
    check = 1;
    return check;
  }
  else{
      if(nack_buffer[pid][0].timer==0){ // if the timer of the first element is zero, then service this request
           // l1messagecounter[pid][nack_buffer[pid][0].message]++;
           //l2messagecounter[get_bankid(nack_buffer[pid][0].memory_addr)][nack_buffer[pid][0].message2]++;
           reference response;
           response.memory_add = nack_buffer[pid][0].memory_addr;
           response.message = nack_buffer[pid][0].message2;
           response.pid = pid;
           l2bank[get_bankid(nack_buffer[pid][0].memory_addr)].push(response); // sending the reply from the nack buffer

           auto iterator = nack_buffer[pid].begin();
           nack_buffer[pid].erase(iterator); // deleting the first element of the buffer

           for(int i=0;i<nack_buffer[pid].size();i++ )
            {
              nack_buffer[pid][i].timer--;  // decrementing the timer of all the message which are in the nack buffer of the private cache
            }
            return check;
       }
        else{// if the timer of first element is not zero

          //decrement the timer of all the request in the nack buffer
           for(int i =0 ; i<nack_buffer[pid].size();i++){
             nack_buffer[pid][i].timer--;
             check=1;
           }
           return check;

        }

  }
}

// function to remove a block entry from mht
void remove_mht(ull memory_addr,int mode,int pid){
 ull block_number = memory_addr/64;
  if(!(mht[pid].find(block_number)==mht[pid].end())){ // if the mode is load then if stored mode is store then don't remove the block else remove the block
       if(mode==mht[pid][block_number]||mode==0) // mode = 0 (store) then remove, or remove when modes of both are same
            mht[pid].erase(block_number);
    }
}

// function to check in mht and add the block in mht if not found (On Miss)
int check_add_mht(ull memory_addr,int mode,int pid){  // mode = 0 (store) and mode = 1 load
    ull block_number = memory_addr/64;
    int check = -1;
    if(mht[pid].find(block_number)==mht[pid].end()){ // if block not found in mht
     mht[pid][block_number]=mode;
         check = 2;
         return check;
    }
    else{
        if((mode==mht[pid][block_number])||mode==1) // check in mht, if block is found in mht

          {
              l1_hits[pid]++;
              if(mode==0)
               {
                 l1_write_hits[pid]++;
               }
               else
               {
                 l1_read_hits[pid]++;
               }
               

           }

        else  {
          l1_upgrade[pid]++;
          reference rqst;
          rqst.memory_add = memory_addr;
          rqst.message = 3;
          rqst.pid = pid;
          l2bank[pid].push(rqst);
          mht[pid][block_number]=mode;  //setting mode to store
        }
       check = 1;
       return check;
    }

}


// function to initialize the L1 and L2 Caches and messagecounter[].
void init(){
    //initializing the L1 Cache with 0 value
    for(int pid = 0; pid<8; pid++){
      for(int index =0; index<64; index++){
        for( int line =0 ; line < 8; line++){
            l1cache[pid][index][line] = 0;
        }
      }
    }

    //initializing the L2 Cache with 0 value

      for(int index =0; index < 4096; index++){
        for( int line =0 ; line < 16; line++){
            l2cache[index][line] = 0;
        }
      }

    for(int i =0; i <8; i++){//Initializing l1 message counter to zero.
      for(int j=0; j<12; j++)
        l1messagecounter[i][j] = 0;
    }


    for(int i =0; i<12;i++){ //Initializing l2message counter to zero.
      l2messagecounter[i] = 0;
    }
}

//Function to invalidate the cache line from the l1 cache.
void invalidate_cacheline(ull address,int pid){
  //Cache line needs to be assigned 0. LRU needs to be updated.
  ull blocknumber = address/64;
  int address_index = blocknumber %64;
  int invalidate_line;
  int flag = 0;
  //Finding the line in which this block is present in cache L1.
  for(int line =0; line<8; line++){
    if(blocknumber == l1cache[pid][address_index][line]/64){
      invalidate_line = line;
      flag=1;
      break;
    }
  }
  if(flag==1){

   l1cache[pid][address_index][invalidate_line] =0; // invalidated the cache line by storing value 0 in it.
   lru.lru1Invalidate(pid,address_index,invalidate_line); // invalidating the lru of that l1 cache.
   }
}

//Function to update the lru of the l1 cache on recieving put and putx messages.
void l1cache_update(int pid, ull address){
  ull blocknumber = address/64;
  int index = int(blocknumber%64);
  int flag_hit = 0;
   for( int line =0 ; line<8 ; line++){
      if(blocknumber == (l1cache[pid][index][line]/64)) {
          flag_hit =1;
          l1_hits[pid]++;//incrementing l1 cache hit count
          lru.lru1Update(pid,index,line);//updating lru data structure for block hit
          break;
      }
  }

  if(!flag_hit){
    printf("line 253 somethings wrong in l1 cache\n");
      // l2_cache(rqst.memory_add);
  }
}

//return value: 1->hit in cache;  0->miss in cache;
int checkl1cache(int pid,trace_request rqst){
  ull blocknumber = rqst.memory_add/64;
  int index = int(blocknumber%64);
  int state = directory.checkstate(blocknumber);
  int flag_hit = 0;
  l1_accesses[pid]++;
   for( int line =0 ; line<8 ; line++){
      if(blocknumber == (l1cache[pid][index][line]/64)) {
          if(rqst.load_store || (state==1 && pid == directory.get_modified_pid(blocknumber))){
          flag_hit =1;   // when block is in cache and no upgrade miss
          l1_hits[pid]++;//incrementing l1 cache hit count
          lru.lru1Update(pid,index,line);//updating lru data structure for block hit
         
          if(rqst.load_store==0){ // if request is write
            l1_write_hits[pid]++;
          }
          else
          {
            l1_read_hits[pid]++;  // if request is read
          }
          
          break;
         }
        else
          { flag_hit = 2; // when there is upgrade miss.
            int k = check_add_mht(rqst.memory_add,rqst.load_store,rqst.tid);
            //if(k==2)
              //l1_upgrade[pid]++;

          }
      }

  }
  //Calling L2 cache in case of a L1 miss
  if(!flag_hit){
     int check= check_add_mht(rqst.memory_add,rqst.load_store,rqst.tid); //load=1 and store=0
      if(check == 2)
        { l1_misses[pid]++;
          if(rqst.load_store==0)
           {
             l1_write_miss[pid]++;
           }
          else
             l1_read_miss[pid]++;
        }
      else
        flag_hit = 3; // no action to be taken when flag-hit is 3 as it is handled in check_add_mht
         //l2_cache(rqst.memory_add);
  }
  return flag_hit;
}



/*Function to update the directory when block is evicted from the L1 cache of processor pid.
  It also increment the count of the REPL_S messages.
  pid: pid of the processor whose block is evicted.*/
void l1_eviction_updatedirectory(ull address, int pid){
  ull blocknumber = address /64;
  l2messagecounter[0]++; //incrementing the REPL_S count
  int state = directory.checkstate(blocknumber);
  if(state == 0){ //block in S state. Removing the pid from sharers list.
    directory.removesharer(blocknumber,pid);
  }
  else if(state ==1){//block in M state. Calling invalidateblock()
    directory.invalidateblock(blocknumber);//-----------------------------------------------------------check if correct----------------------------
    directory.modified.erase(blocknumber);
  }
}
//function to implement inclusion policy for adding a block from l2 cache to l1 cache and updating the directory on eviction.
void inclusion_policy_insert_in_l1(int pid,unsigned long long address){
    unsigned long long block_number;
    block_number = address / 64;
    int address_index = int(block_number % 64);
    int replace_line = lru.lru1GetLine(pid,address_index);//getting line which needs to be replaced with the new block from l1 cache
    ull replaced_address = l1cache[pid][address_index][replace_line];
    if(replaced_address){ //If cache line was not empty
      //Directory needs to be updated on eviction.
      l1_eviction_updatedirectory(replaced_address,pid);
    }

    //Inserting new block in l1 cache
    l1cache[pid][address_index][replace_line] = address;
    lru.lru1Update(pid,address_index, replace_line);

}

// function to implement inclusion policy for L3 cache block eviction
void inclusion_policy_l2_evict(unsigned long long l2_replaced_address){
// Checking whether replaced block is present in l1 cache or not
    //Obtaining address_index for l2 cache
    unsigned long long blocknumber;
    blocknumber = l2_replaced_address / 64;
    int address_index = int(blocknumber % 64);
    // All those processors which have this block in their L1 cache needs to invalidate the block
    //Updating the directory after these invalidations
    int state = directory.checkstate(blocknumber);
    list<int> pid_list;
    if(state ==0){ //block in S state
      pid_list = directory.get_sharers_pid(blocknumber);
    }
    else if(state ==1){ // block in M state
      pid_list.push_back(directory.get_modified_pid(blocknumber));
    }

    /* Sending Invalidations to the L1 caches that stores this evicted block.
    We will append invalidation message to the l1_input_queue of the processors whose pid is in pid_list.*/
    reference rqst;
    rqst.memory_add = l2_replaced_address;
    rqst.message = 6;
    for(list<int>::iterator it = pid_list.begin(); it!=pid_list.end(); ++it){
      rqst.pid= *it;
      l1_input[*it].push(rqst);
    }

}

// function to handle L2 cache mises and bringing the block from Main Memory.
void memory(int bankid, reference rqst, int address_index){
    unsigned long long address = rqst.memory_add;
    // Find the cache line number which needs to be replaced from L2 cache
    int replace_line = lru.lru2GetLine(bankid,address_index);
    unsigned long long l2_replaced_address = l2cache[address_index][replace_line];
    // If Replaced block is a valid block then invalidate that block in l1 cache (if present) - inclusion policy
    if(l2_replaced_address){
        //calling function to implement inclusion policy for block evicted from l2 cache.
        inclusion_policy_l2_evict(l2_replaced_address);
    }

    //Placing the missed block from M.M. to l2 Cache
    l2cache[address_index][replace_line] = address;
    lru.lru2Update(bankid,address_index,replace_line);

    // Providing this new block to L1 cache through inclusion policy.
     // inclusion_policy_insert_in_l1(rqst.pid,address);
}

void l2cache_update(reference head){
  l2accesses++;
  ull blocknumber = head.memory_add/64;
  int index = int(blocknumber%4096);

  //search in l2 cache.
  int flag_hit = 0;
  for( int line =0 ; line < 16; line++){
    if(blocknumber == (l2cache[index][line]/64)) {
        flag_hit =1;
        l2hits++;
        lru.lru2Update(0,index,line);  //updating lru data structure for block hit
         break;
    }
  }
  if(!flag_hit){
    printf("somethings wrong from l2 cache\n");
  }
}

//Function to search in L2 cache
void checkl2cache(reference head){
  l2accesses++;
  int bankid = get_bankid(head.memory_add);
  ull blocknumber = head.memory_add/64;
  int index = int(blocknumber%4096);

  //search in l2 cache. if hit then provide the block. if miss then perform the replacement policy.
  //Checking L3 cache hit or miss
  int flag_hit = 0;
  for( int line =0 ; line < 16; line++){
    if(blocknumber == (l2cache[index][line]/64)) {
        flag_hit =1;
        l2hits++;
        lru.lru2Update(bankid,index,line);  //updating lru data structure for block hit
        // inclusion_policy_insert_in_l1(head.pid,head.memory_add);//inserting the block in L1 Cache of processor head.pid.
         break;
    }
  }
  //Handling L2 miss by calling memory function
  if(!flag_hit){
      l2misses++;
      memory(bankid,head, index);
  }

}
// function to process GET request by a processor head.pid.
void GET(reference head){
  int state = directory.checkstate(head.memory_add/64);
  int p_state = directory.get_pstate(head.memory_add/64);
  //if p_state is not set for the block
     //Here block can only be in S state or in M state. If it was in I state request would have been GETX not GET.
     if(state == 0){// block is in S state. Providing the block from L2 cache.
         checkl2cache(head); //calling it to update the lru of l2 cache and providing the block to the requester.
         // l1messagecounter[head.pid][7]++;// Incrementing the PUT message counter.
         add_put_message(head.pid, head.memory_add); //Sending PUT message to the l1input queue.
         directory.addsharers(head.memory_add/64,head.pid); //adding the processor into sharers' list.
      }
      else if(state == 1){// block is in M state. Sending XFER request to the owner of the block(dirty_pid). And setting the pending state.
        int dirty_pid = directory.get_modified_pid(head.memory_add/64);
        reference rqst;
        rqst.memory_add = head.memory_add;
        rqst.pid = head.pid;
        rqst.message = 5;
        l1_input[dirty_pid].push(rqst);
        directory.set_pstate(head.memory_add/64);//Enter into P state.
      }
      else{ //block is in I state. This can only occur in the case of race.
        printf("Race: GET request when block is in I state\n");
        checkl2cache(head); //calling it to update the lru of l2 cache and providing the block to the requester.
        // l1messagecounter[head.pid][7]++;// Incrementing the PUT message counter.
        add_put_message(head.pid, head.memory_add); //Sending PUT message to the l1input queue.
        directory.addsharers(head.memory_add/64,head.pid); //adding the processor into sharers' list.
      }
}

// Function to send the invalidation messages to all the sharers and send PUTX as response to the requester
void send_invalidations_to_sharers(reference head){
  ull blocknumber = head.memory_add/64;
  reference rqst;
  rqst.memory_add = head.memory_add;
  //Getting list of sharers
  list<int> pid_list = directory.get_sharers_pid(blocknumber);
  // Directory will append to the l1_input_queue of the processors whose pid is in pid_list. (Sending Invalidations)
  rqst.message = 6;
  rqst.pid= head.pid;
  for(list<int>::iterator it = pid_list.begin(); it!=pid_list.end(); ++it){
    l1_input[*it].push(rqst); //Added invalidation message to the l1_input queue.
    directory.removesharer(blocknumber,*it);
  }
  //Sending PUTX response to the requester. Requester will get the number of sharers and then accept their INVACK
  /*reference putxresponse;
  putxresponse.message = 11;
  putxresponse.sharers = pid_list.size();
  l1_input[head.pid].push(putxresponse);*/ //Added the PUTX response, where count of number of sharers is sent.
}

// Function when GETX message needs to be processed by L2 queue
void GETX(reference head){
  //Here block can be in I state, S state or in M state.
  ull blocknumber = head.memory_add/64;
  int state = directory.checkstate(blocknumber);
  int p_state = directory.get_pstate(head.memory_add/64);
  reference rqst;
  rqst.memory_add = head.memory_add;

    if(state== -1){ //block is in I state. Providing the block from L2 cache.
      checkl2cache(head);//calling it to update the lru of l2 cache and providing the block to the requester.
      add_putx_message(head.pid, head.memory_add); //Sending PUTX message to the l1input queue.
      directory.addmodified(blocknumber, head.pid);//Adding the block in M State.
      // l1messagecounter[head.pid][11]++;// Incrementing the PUTX message counter.
    }
    else if(state == 0){// block is in S state. Providing the block from L2 cache and sending invalidations to sharers.
      /*Provide the block from L2 cache; Change the state of the block to M.
        Send invalidation messages to the sharers. Send Putx response message to the requester.*/
      checkl2cache(head); //calling it to update the lru of l2 cache and providing the block to the requester.
      add_putx_message(head.pid, head.memory_add); //Sending PUTX message to the l1input queue.
      // l1messagecounter[head.pid][11]++;// Incrementing the PUTX message counter.
      directory.addmodified(blocknumber, head.pid);//Adding the block in M State.
      send_invalidations_to_sharers(head);// Send invalidation messages to the sharers and PUTX to the requester.
    }
    else if(state == 1){// block is in M state. Pushing a XFER_INV request inside the l1_input queue of processor having the block in M state.
      int dirty_pid = directory.get_modified_pid(blocknumber);
      //directory.addmodified(blocknumber, head.pid);//Adding the block in M State.
      rqst.pid = head.pid;
      rqst.message = 10;
      l1_input[dirty_pid].push(rqst);
      directory.set_pstate(blocknumber);//Enter into P state.
    }


}

// Function when UPGRADE message needs to be processed by L2 queue
void UPGRADE(reference head){ //--------------------------------------------------------write here---------------------------
  //Here block can be in I state(race condition), S state or in M state(race condition).
  ull blocknumber = head.memory_add/64;
  reference rqst;
  rqst.memory_add = head.memory_add;
  int state = directory.checkstate(blocknumber);

    if(state== -1){ //block is in I state. Providing the block from L2 cache.
    checkl2cache(head);//calling it to update the lru of l2 cache and providing the block to the requester.
    directory.addmodified(blocknumber, head.pid);//Adding the block in M State.
    // l1messagecounter[head.pid][11]++;// Incrementing the PUTX message counter.
    add_putx_message(head.pid, head.memory_add); //Sending PUTX message to the l1input queue.
   }
   else if(state == 0){// block is in S state. send invalidations to sharers.
    /*Change the state of the block to M.
      Send invalidation messages to the sharers. Send Putx response message to the requester.*/
    directory.addmodified(blocknumber, head.pid);//Adding the block in M State.
    send_invalidations_to_sharers(head); // Send invalidation messages to the sharers and PUTX to the requester.

    px(head.pid,head.memory_add); // calling px which will update the l1 lru and increment putx message.
    checkl2cache(head); // the block is send by l2 cache, hence l2 cache should also be updated.
   }
   else if(state == 1){// block is in M state. Pushing a XFER_INV request inside the l1_input queue of processor having the block in M state.
    int dirty_pid = directory.get_modified_pid(blocknumber);
    //directory.addmodified(blocknumber, head.pid);//Adding the block in M State.
    rqst.pid = head.pid;
    rqst.message = 10;
    l1_input[dirty_pid].push(rqst);
    directory.set_pstate(blocknumber);//Enter into P state.
    }

}

//Function to perfrom Sharing Write Back to the L2 bank.
void shared_wb(ull address){
  //Adding SWB message into l2 bank queue.
  reference response;
  response.memory_add = address;
  response.message = 4;
  // response.pid = -1;
  l2bank[get_bankid(address)].push(response);
}

//Function to send the put message in the l1_input queu.
void add_put_message(int pid, ull address){
  reference message;
  message.memory_add = address;
  //message.pid=  5;
  message.message = 7;
  l1_input[pid].push(message);
}

//Function to send the putx message in the l1_input queu.
void add_putx_message(int pid, ull address){
  reference message;
  message.memory_add = address;
  //message.pid= 5;
  message.message = 11;
  l1_input[pid].push(message);

  msg_putx[pid]++;
}
// Function to send the putx message and updating the l1 cache. It does so by calling to functions.
void px(int pid, ull address){
  add_putx_message(pid, address); //Sending PUTX message to the l1input queue.
  // l1messagecounter[pid][11]++; // Counting a PUTX operation.
  // l1cache_update(pid,address); // updating lru of l1 cache
}

void p(int pid, ull address){
  // l1messagecounter[pid][7]++; // Counting a PUT operation.
  add_put_message( pid,  address); //Sending PUT message to the l1input queue.
  // l1cache_update(pid, address); // updating lru of l1 cache
}
void PUTX(int pid, ull address){
  // inclusion_policy_insert_in_l1(pid,address);// Enter the block inside the l1 cahce of requester(pid.)
  // l1messagecounter[pid][11]++; // Counting a PUTX operation.
    add_putx_message(pid, address); //Sending PUTX message to the l1input queue.
}

void PUT(int pid, ull address){
  // inclusion_policy_insert_in_l1(pid,address);// Enter the block inside the l1 cahce of requester(pid.)
  // l1messagecounter[pid][7]++; // Counting a PUT operation.
  add_put_message( pid,  address); //Sending PUT message to the l1input queue.
}

/*function to implement the XFER operation.=> Provide the block to another processor; Move to S staete; Perfrom Shared_WB.
  dirty_pid: pid of the processor whose block needs to be provided to another processors*/
void XFER(reference head, int dirty_pid){
  directory.addsharers(head.memory_add/64,dirty_pid); // Transfering the state from M to S.
  PUT(head.pid,head.memory_add); //putting the block inside the requester's l1 cache.
  directory.addsharers(head.memory_add/64,head.pid); // Adding this processor into S state.
  shared_wb(head.memory_add);//Performing Shared_WB.
}
/*function to implement the XFER_INV operation.=> Provide the block to another processor; Move to I staete;
  Invlaidate the cache line; Perfrom Shared_WB.
  dirty_pid: pid of the processor whose block needs to be provided to another processors*/
void XFER_INV(reference head, int dirty_pid){
  directory.invalidateblock(head.memory_add/64); // Invalidating the block in directory
  invalidate_cacheline(head.memory_add,dirty_pid);//Invalidating the cache line of dirty_pid processor. Moving to I state.
  PUTX(head.pid,head.memory_add); //putting the block inside the requester's l1 cache.
  shared_wb(head.memory_add);//Performing Shared_WB.
  directory.addmodified(head.memory_add/64, head.pid);
}


// function which returns the id of home node of a memory address
int get_bankid(ull memory_add){
  ull block_number = memory_add/64;
  int bankid = block_number % 8;
  return bankid;
}

//Function to process the l1_trace queue
void l1trace_process(int pid){
  reference temp;
  if(!l1_trace[pid].empty()){//if queue is not empty
    // Popping l1_trace queue
    trace_request rqst = l1_trace[pid].front();
    l1_trace[pid].pop();

    int bankid = get_bankid(rqst.memory_add); // getting home node of the missed block.
    //Copying the values in temp variable. This temp variable will be pushed into l2bank[] queue.
    temp.memory_add = rqst.memory_add;
    temp.pid = pid;
     
     // incrementing counter on the basis of whether a request is load or store (read or write)
     if(rqst.load_store==0){
       l1_write[pid]++;  // if request is write
     }
     else
     {
       l1_read[pid]++; // if request  is read
     }
     
    /*
    Process trace (queue1 of L1) request
    */
    int state = directory.checkstate(rqst.memory_add/64);
    if(rqst.load_store){// Load Request. Need to send GET message if block misses in the l1cache. And GETX if no one has the block. i.e. E state
      if(!checkl1cache(pid,rqst)){ // if cache misses.
        //Pushing GET request inside home nodes queue (l2bank[bankid].)
        if(state == -1) //Block is in I state.
          temp.message = 2; // So message will be GETX.
        else //Block is not in I state.
          temp.message = 1; //So message will be GET.
        l2bank[bankid].push(temp);
      }
    }
    else{// Store Request. So need to send GETX message if block is not in M state. UPGRADE if block is in S state.
      //Checking whether block is in M State inside this Processors cache
      int c =checkl1cache(pid,rqst);
      if(c==1){ // if L1 cache hits.
        if(state == 1 && pid == directory.get_modified_pid(rqst.memory_add/64)){
           //block is present in the this cache only in M state.
            //Do nothing. Processor will write into the block.

        }
        else{
            cout<<"I am not owner, make changes line 730"<<endl;
        }
      }
        else if(c==2){ //block is present in the cache in S state.
            temp.message = 3;//Sending UPGRADE request to home node.
            l1_upgrade[pid]++;
            l2bank[bankid].push(temp);//Pushing this request inside home nodes queue (l2bank[bankid])
        }

      else if (c==0){ //L1 cache misses. Just send GETX request.
        temp.message = 2;//Sending GETX request to home node.
        l2bank[bankid].push(temp);//Pushing this request inside home nodes queue (l2bank[bankid])
      }
    }
  }
}

//Function to process the l1_input queue
void l1_input_process(int pid){
  /*
  Process l1input[] queue (queue2 of L1) request
  It will recieve only XFER, XFER_INV, INVALIDATION, PUTX, and INVACK requests.
  */
  if(cycle){ //if this is not first cycle;

       if(!l1_input[pid].empty()){//if queue is not empty
      //Popping l1_input queue
      reference rqst_coherence = l1_input[pid].front();
      l1_input[pid].pop();
      if(rqst_coherence.message!=11)
      l1messagecounter[pid][rqst_coherence.message]++;


      if(rqst_coherence.message==9){
        msg_nack n;
        n.timer = 10;
        n.memory_addr = rqst_coherence.memory_add;
        n.message  =rqst_coherence.message;
        n.message2 =rqst_coherence.message2;
        nack_buffer[pid].push_back(n);
      }

      if(rqst_coherence.message == 5){ // Request for XFER
        XFER(rqst_coherence,pid); //Calling XFER with the rqst_coherence and the pid of this processor (the processor which holds the block in M state.)
      }
      else if(rqst_coherence.message == 10){ // Request for XFER_INV
        XFER_INV(rqst_coherence,pid); //Calling XFER_INV with the rqst_coherence and the pid of this processor (the processor which holds the block in M state.)
      }
      else if(rqst_coherence.message == 6){ // Request for INVALIDATION
        //Cache line needs to be assigned 0. LRU needs to be updated. Directory needs to be updated
        invalidate_cacheline(rqst_coherence.memory_add,pid); //Invalidating the cache line of L1 cache

        //Sending Invalidation Acknowledgement
        reference invresponse;
        invresponse.message = 8;
        invresponse.pid = pid;
        l1_input[rqst_coherence.pid].push(invresponse);//Added the invalidation acknowledgement in l1_input queue.
      }
      else if(rqst_coherence.message == 11){ // PUTX Response message received. L1 cache needs to store this received block in it and update LRU.
        inclusion_policy_insert_in_l1(pid,rqst_coherence.memory_add);
        l1messagecounter[pid][11]++;
        remove_mht(rqst_coherence.memory_add,0,pid);

      }
      else if(rqst_coherence.message == 7){ // PUT Response message received. L1 cache needs to store this received block in it and update LRU.
       inclusion_policy_insert_in_l1(pid,rqst_coherence.memory_add);
        remove_mht(rqst_coherence.memory_add,1,pid);

      }
      else if(rqst_coherence.message == 8){ // INVACK Response message
        //Do nothing as we don't need to write any data in the cache.
       }
     }



  }
}

//Function to process the l2_input queue
void l2bank_process(int pid){
  /*
  Process l2bank[pid] (queue of L2bank) request
  */
  if(cycle){ //if this is not first cycle;
    if(!l2bank[pid].empty()){ //if queue is not empty
      //Popping l2bank queue
      reference l2rqst = l2bank[pid].front();
      l2bank[pid].pop();
      l2messagecounter[l2rqst.message]++;
      ull blocknumber = l2rqst.memory_add/64;
      int state = directory.checkstate(blocknumber); //getting state of the block
      int pstate =directory.get_pstate(blocknumber);
      if(pstate==0){  // if pstate is reset

       // It can recieve GET, GETX, UPGRADE, and SHARING_WB requests.
        if(l2rqst.message == 1){ // Request for GET
          GET(l2rqst);
        }
        else if(l2rqst.message == 2){ // Request for GETX
          GETX(l2rqst);
        }
        else if(l2rqst.message == 3){ // Request for UPGRADE
          UPGRADE(l2rqst);
        }
        else if(l2rqst.message == 4){ // SWB Response message
          // Need to reset the P bit.
          directory.reset_pstate(l2rqst.memory_add/64);
          // l2cache_update(l2rqst);

        }
      }
      else{ // if pstate is set then send nack for GET,GETX and UPGRADE and reset p_state if SWB comes

         if(l2rqst.message==4){ // if message is SWB then reset p_state
            directory.reset_pstate(l2rqst.memory_add/64);
         }
         else  {
            send_nack(l2rqst);
         }

      }

    }
  }
}

// Function to simulate a processor
void processor(int pid){
  reference temp; 
  int flag = 0;
  //Calling functions to process all 3 queues one by one.
  if(!l1_trace[pid].empty())
   { if(total_count==l1_trace[pid].front().count)
        { int check = l1_check_nack(pid); // 1- no request from nack buffer is serviced, 0-request from nack buffer serviced
          if(check){ // if nothing is serviced from the nack buffer then only serive request from l1_trace queue
           l1trace_process(pid);
         }
          else{ // if request from nack buffer is serviced then decrement total_count as L1 trace is not serviced.
            flag=1;
            total_count--;
          }

        }
      
   }
  if(flag==0){
      int check = l1_check_nack(pid); // if the above condition fails then check nack buffer.
    }
  l1_input_process(pid);
  
  l2bank_process(pid);
}

//Compare function to sort array of structure trace_request.

int compare (const void * a, const void * b)
{

  const trace_request *requestA = (trace_request *)a;
  const trace_request *requestB = (trace_request *)b;

  return ( requestA->count - requestB->count);
}

int main(){
  //int tid;

  //reading trace file
  FILE *fp = fopen("addrtrace.txt", "r");
  assert(fp != NULL);
  int k =0;
  while(1){
     trace_request machine_access;
     fscanf(fp,"%d %d %llu %llu",&machine_access.tid, &machine_access.load_store,&machine_access.count,&machine_access.memory_add);
     if(machine_access.tid<0 || machine_access.tid>7)
       break;
       k++;
     //store them in l1_trace[i]
     l1_trace[machine_access.tid].push(machine_access);
  }
  cout<<"Done Reading File !!"<<endl;
  //Done Reading File!

  int terminate =0; // if value =8, then program needs to terminate as all the queues are empty
  init();
  lru.init();
  //int k =0 ;
  while(terminate<8){
    int access[8]={0};
    trace_request temp [8]; // temp array of structure trace_request
    int entries=0; // variable to store number of entries in temp.
    for(ull i=0;i<8;i++){
      if(!l1_trace[i].empty())
          {
            temp[entries] = l1_trace[i].front();
            entries++;
            //if(k<3)
            //cout<<temp[entries-1].count<<" "<<endl;
          }

      }
     //qsort for sorting array of structure temp
     qsort(temp,entries,sizeof(trace_request),compare);

     //Debugging
     /*if(k < 3)
      {
        for(int i =0 ;i<entries;i++)
           cout<<temp[i].count << " ";
         cout<<endl;
         k++;
      }*/

     for(int i =0 ;i < entries;i++)
      {
          if(total_count==temp[i].count )
            { //cout<<"Hello"<<endl;
              processor(temp[i].tid);
              total_count++;

              //debugging 
              //cout<<total_count<<endl;
              //cout<<"total_count reached :"<<total_count-1<<endl;
              
              access[temp[i].tid]=1;
            }
          else
            break;


      }

      for(int i=0;i<8;i++)
       {
          if(access[i]!=1)
            processor(i);

       }

    cycle++;
    terminate =0;
    for(int i =0;i<8;i++){
      if(l1_trace[i].empty() && l1_input[i].empty() && l2bank[i].empty()){
        terminate++;
      }
    }
  }
  ofstream fout;
  // char outputfilename[100];
	// sprintf(outputfilename,"output.csv");
  fout.open("output.csv");

  fout<<"Total Machine Accesses," <<k<<endl;
  fout<<"Total Cycles,"<<cycle<<endl;
  //fout<<"Total global count,"<<total_count<<endl<<endl;
  fout<<"Messages\n";
  fout<<"L1 Input Que Message Counts Processor wise\n";
  fout<<"Core  Number,REPL_S,GET,GETX,UPGRADE,SWB,XFER,INV,PUT,INVACK,NACK,XFER_INV,PUTX"<<endl;
  for(int i =0; i<8; i++){
      fout<<"Core "<<i<<":,";
    for (int j=0; j<12; j++){
      fout<<l1messagecounter[i][j]<<",";
    }
    fout<<endl;
  }

  fout<<endl;

  fout<<"L2 Input Que Message Count.\n";
  fout<<"L2,";
  for(int i =0; i<12; i++)
    fout<<l2messagecounter[i]<<",";
    fout<<endl<<endl;

  fout<<"L1 Cache Details "<<endl;

  fout<<"Core Number,L1 accesses,L1 cache misses,L1 cache hits,L1 upgrade misses\n";
  for(int i =0 ; i<8; i++){
    fout<<"Core "<<i<<":,"<<l1_accesses[i]<<","<<l1_misses[i]<<","<<l1_hits[i]<<","<<l1_upgrade[i]<<endl;
  }
  fout<<endl<<endl;
  fout<<"L1 cache Details in terms of read miss, write miss and upgrade miss"<<endl;
  fout<<"Core\t Read  Accesses\t Write Accesses\t Read Hits\t Write Hits\t Read Misses\t Write Misses\t Upgrade Miss"<<endl;
  for(int i=0;i<8;i++){
    fout<<"Core "<<i<<":,"<<l1_read[i]<<","<<l1_write[i]<<","<<l1_read_hits[i]<<","<<l1_write_hits[i]<<","<<l1_read_miss[i]<<","<<l1_write_miss[i]<<","<<l1_upgrade[i]<<endl;
  }

  fout<<endl<<endl;
  fout<<"L2 Cache Details:,"<<endl;
  fout<<"L2 accesses:,"<<l2accesses<<endl;
  fout<<"L2 hits:,"<<l2hits<<endl;
  fout<<"L2 misses:,"<<l2misses<<endl;


  cout<<"--------------------------------------------------------------------------"<<endl;
  cout<<"Total Machine Accesses :" <<k<<endl;
  cout<<"Total Cycles :"<<cycle<<endl;
  //cout<<"Total global count:"<<total_count<<endl;
  cout<<"--------------------------------------------------------------------------"<<endl;
  printf("Messages\n");
  cout<<"--------------------------------------------------------------------------"<<endl;
  printf("L1 Input Que Message Counts Processor wise\n");
  cout<<"--------------------------------------------------------------------------"<<endl;
  cout<<"Core\tREPL_S\tGET\tGETX\tUPGRADE\tSWB\tXFER\tINV\tPUT\tINVACK\tNACK\tXFER_INV PUTX"<<endl;
  for(int i =0; i<8; i++){
      cout<<"Core "<<i<<":\t";
    for (int j=0; j<12; j++){
      cout<<l1messagecounter[i][j]<<"\t";
    }
    cout<<endl;
  }
  cout<<"--------------------------------------------------------------------------"<<endl;
  printf("L2 Input Que Message Count.\n");
  cout<<"--------------------------------------------------------------------------"<<endl;

    cout<<"L2 \t";
    for(int i =0; i<12; i++)
      cout<<l2messagecounter[i]<<"\t";
      cout<<endl;

  cout<<"--------------------------------------------------------------------------"<<endl;
  cout<<"L1 Cache Details "<<endl;
  cout<<"--------------------------------------------------------------------------"<<endl;

  cout<<"Core\tL1 accesses\tL1 cache misses\tL1 cache hits\tL1 upgrade misses \n";
  for(int i =0 ; i<8; i++){
    cout<<"Core "<<i<<":\t"<<l1_accesses[i]<<"\t \t"<<l1_misses[i]<<"\t \t"<<l1_hits[i]<<"\t \t"<<l1_upgrade[i]<<endl;
  }
  cout<<"--------------------------------------------------------------------------"<<endl;
  cout<<"L1 cache Details in terms of read miss, write miss and upgrade miss"<<endl;
  cout<<"---------------------------------------------------------------------------------------------------------------------"<<endl;
  cout<<"Core\t Read Accesses\tWrite Accesses\tRead Hits\tWrite Hits\tRead Misses\t Write Misses\t Upgrade Miss"<<endl;
  cout<<"---------------------------------------------------------------------------------------------------------------------"<<endl;
  for(int i=0;i<8;i++){
    cout<<"Core "<<i<<":\t "<<l1_read[i]<<"\t \t"<<l1_write[i]<<"\t \t"<<l1_read_hits[i]<<"\t \t"<<l1_write_hits[i]<<"\t \t"<<l1_read_miss[i]<<"\t  \t "<<l1_write_miss[i]<<"\t  \t "<<l1_upgrade[i]<<endl;
  }




  cout<<"----------------------------------------------------------------------------------------------------------------------"<<endl;
  cout<<" L2 Cache Details"<<endl;
  cout<<"--------------------------------------------------------------------------"<<endl;
  cout<<" L2 accesses: "<<l2accesses<<endl;
  cout<<" L2 hits: "<<l2hits<<endl;
  cout<<" L2 misses: "<<l2misses<<endl;
  cout<<"--------------------------------------------------------------------------"<<endl;
  cout<<"output.csv generated"<<endl;
  cout<<"--------------------------------------------------------------------------"<<endl;


  return 0;
}
