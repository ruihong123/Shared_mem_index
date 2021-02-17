#include <iostream>
#include "rdma.h"
void client_thread(RDMA_Manager* rdma_manager){

  auto myid = std::this_thread::get_id();
  std::stringstream ss;
  ss << myid;
  auto* thread_id = new std::string(ss.str());
  rdma_manager->Remote_Memory_Register(100*1024*1024);
  rdma_manager->Remote_Query_Pair_Connection(*thread_id);
  std::cout << rdma_manager->remote_mem_pool[0];
  ibv_mr mem_pool_table[2];
  mem_pool_table[0] = *(rdma_manager->local_mem_pool[0]);
  mem_pool_table[1] = *(rdma_manager->local_mem_pool[0]);
  mem_pool_table[1].addr = (void*)((char*)mem_pool_table[1].addr + sizeof("message from computing node"));// PROBLEM Could be here.

  char *msg = static_cast<char *>(rdma_manager->local_mem_pool[0]->addr);
  strcpy(msg, "message from computing node");
  int msg_size = sizeof("message from computing node");
//  rdma_manager->RDMA_Write(rdma_manager->remote_mem_pool[0], &mem_pool_table[0],
//                           msg_size, *thread_id);
//
//  rdma_manager->RDMA_Read(rdma_manager->remote_mem_pool[0], &mem_pool_table[1],
//                          msg_size, *thread_id);
//  ibv_wc* wc = new ibv_wc();
//
//    rdma_manager->poll_completion(wc, 2, thread_id);
//    if (wc->status != 0){
//      fprintf(stderr, "Work completion status is %d \n", wc->status);
//      fprintf(stderr, "Work completion opcode is %d \n", wc->opcode);
//    }


  std::cout << "write buffer: " << (char*)mem_pool_table[0].addr << std::endl;

  std::cout << "read buffer: " << (char*)mem_pool_table[1].addr << std::endl;

}

int main()
{
  struct config_t config = {
      NULL,  /* dev_name */
      NULL,  /* server_name */
      19875, /* tcp_port */
      1,	 /* ib_port */ //physical
      1, /* gid_idx */
      4*10*1024*1024 /*initial local buffer size*/
  };


  auto Remote_Bitmap = new std::map<void*, In_Use_Array>;
//  ibv_mr inuse_mr;
//  inuse_mr.length = 559;
//  In_Use_Array inuse_a(100, 1000, &inuse_mr);
//  Remote_Bitmap->insert({nullptr, inuse_a});
  size_t table_size = 8*1024*1024;
  RDMA_Manager* rdma_manager = new RDMA_Manager(
          config, Remote_Bitmap, table_size);
  rdma_manager->Mempool_initialize(std::string("read"), 8*1024);
  rdma_manager->Mempool_initialize(std::string("write"), 1024*1024);

  rdma_manager->Client_Set_Up_Resources();

  ibv_mr* mr = new ibv_mr;
  In_Use_Array in_use(10,4096,mr);
  for (int i = 0; i<5; i++)
    in_use.allocate_memory_slot();
  Remote_Bitmap->insert({static_cast<void*>(&rdma_manager), in_use});
  size_t buff_size = 1024*1024*1014;
  char* test_buff = static_cast<char*>(malloc(buff_size));
  int mr_flags =
      IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;



  return 0;
}
