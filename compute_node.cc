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

  size_t table_size = 4*1024;
  RDMA_Manager* rdma_manager = new RDMA_Manager(
          config, Remote_Bitmap, table_size);
  rdma_manager->Mempool_initialize(std::string("block"), 4*1024);


  rdma_manager->Client_Set_Up_Resources();
    ibv_mr* send_mr;
    ibv_mr* receive_mr;
    ibv_mr* remote_mr;
    std::string dummyfile("dummy");
    rdma_manager->Allocate_Local_RDMA_Slot(send_mr, "block");
    rdma_manager->Allocate_Local_RDMA_Slot(receive_mr, "block");
    rdma_manager->Allocate_Remote_RDMA_Slot(remote_mr);
    std::string str = "RDMA MESSAGE";
//    memcpy(static_cast<char*>(res->send_buf)+db_name.size(), "\0", 1);
    memcpy(send_mr->addr, str.c_str(), str.size());
    memset((char*)send_mr->addr+str.size(), 0,1);
    // q_id is "" then use the thread local qp.
    rdma_manager->RDMA_Write(remote_mr, send_mr, str.size()+1, "",IBV_SEND_SIGNALED, 1);
    rdma_manager->RDMA_Read(remote_mr, receive_mr, str.size()+1, "",IBV_SEND_SIGNALED, 1);
    printf((char*)receive_mr->addr);


  return 0;
}
