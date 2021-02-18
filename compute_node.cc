#include <iostream>
#include "rdma.h"
//global values for the test
bool test_start = false;
int thread_ready_num = 0;
int thread_finish_num = 0;
size_t thread_num;
std::mutex startmtx;
std::mutex finishmtx;
std::condition_variable cv;


void
client_thread(RDMA_Manager *rdma_manager, int iteration, ibv_mr **local_mr_pointer, ibv_mr **remote_mr, size_t msg_size,
              int r_w) {
    std::unique_lock<std::mutex> lck_start(startmtx);
    rdma_manager->RDMA_Write(remote_mr[0], local_mr_pointer[0],
                             msg_size, "", IBV_SEND_SIGNALED, 1);
    thread_ready_num++;
    if (thread_ready_num >= thread_num) {
        cv.notify_all();
    }
    while (!test_start) {
        cv.wait(lck_start);

    }

    lck_start.unlock();
    for (int i = 0; i < iteration; i++) {
        if ((i + 1) % 10 == 0) {
            int j = (i + 1) % 10;
            if (r_w)
                rdma_manager->RDMA_Write(remote_mr[j], local_mr_pointer[j],
                                         msg_size, "", IBV_SEND_SIGNALED, 1);
            else
                rdma_manager->RDMA_Read(remote_mr[j], local_mr_pointer[j],
                                        msg_size, "", IBV_SEND_SIGNALED, 1);
        } else {
            if (r_w)
                rdma_manager->RDMA_Write(remote_mr[0], local_mr_pointer[0],
                                         msg_size, "", IBV_SEND_SIGNALED, 1);
            else
                rdma_manager->RDMA_Read(remote_mr[0]    , local_mr_pointer[0],
                                        msg_size, "", IBV_SEND_SIGNALED, 1);
        }

    }
    std::unique_lock<std::mutex> lck_end(startmtx);
    thread_finish_num++;
    if (thread_finish_num >= thread_num) {
        cv.notify_all();
    }
    lck_end.unlock();
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
  rdma_manager->Mempool_initialize(std::string("4k"), 4*1024);


  rdma_manager->Client_Set_Up_Resources();
    ibv_mr* send_mr;
    ibv_mr* receive_mr;
    ibv_mr* remote_mr;
    std::string dummyfile("dummy");
    rdma_manager->Allocate_Local_RDMA_Slot(send_mr, "4k");
    rdma_manager->Allocate_Local_RDMA_Slot(receive_mr, "4k");
    rdma_manager->Allocate_Remote_RDMA_Slot(remote_mr);
    std::string str = "RDMA MESSAGE\n";
//    memcpy(static_cast<char*>(res->send_buf)+db_name.size(), "\0", 1);
    memcpy(send_mr->addr, str.c_str(), str.size());
    memset((char*)send_mr->addr+str.size(), 0,1);
    // q_id is "" then use the thread local qp.
    rdma_manager->RDMA_Write(remote_mr, send_mr, str.size()+1, "",IBV_SEND_SIGNALED, 1);
    rdma_manager->RDMA_Read(remote_mr, receive_mr, str.size()+1, "",IBV_SEND_SIGNALED, 1);
    printf((char*)receive_mr->addr);
    printf("multiple threaded test");



    size_t read_block_size;
    std::cin >> read_block_size;
//  table_size = read_block_size+64;
    int readorwrite;
    std::cout << "Read or write:\r" << std::endl;//read 0 write anything else
    std::cin >> readorwrite;

    std::cout << "thread num:\r" << std::endl;
    std::cin >> thread_num;
    rdma_manager->Mempool_initialize(std::string("test"), read_block_size);
    ibv_mr* RDMA_local_chunks[thread_num][10];
    ibv_mr* RDMA_remote_chunks[thread_num][10];
    int iteration = 100;
    long int starts;
    long int ends;
    std::thread* thread_object[thread_num];
    for (size_t i = 0; i < thread_num; i++){
//        SST_Metadata* sst_meta;

        for(size_t j= 0; j< 1; j++){
            rdma_manager->Allocate_Remote_RDMA_Slot(RDMA_remote_chunks[i][j]);

            rdma_manager->Allocate_Local_RDMA_Slot(RDMA_local_chunks[i][j], std::string("test"));
            size_t msg_size = read_block_size;
            memset(RDMA_local_chunks[i][j]->addr,1,msg_size);
        }

    }
    for(int i = 0; i < thread_num; i++)
    {


        thread_object[i] = new std::thread(client_thread, rdma_manager, iteration,
                                           RDMA_local_chunks[i], RDMA_remote_chunks[i], read_block_size, readorwrite);
        thread_object[i]->detach();

    }
    std::unique_lock<std::mutex> l_s(startmtx);
    while (thread_ready_num!= thread_num){
        cv.wait(l_s);
    }
    test_start = true;
    cv.notify_all();
    starts = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    l_s.unlock();
    std::unique_lock<std::mutex> l_e(finishmtx);

    while (thread_finish_num < thread_num) {
        cv.wait(l_e);
    }
    ends  = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    printf("thread has finished.\n");
    l_e.unlock();
    double bandwidth = ((double)read_block_size*thread_num*iteration) / (ends-starts) * 1000;
    double latency = ((double) (ends-starts)) / (thread_num * iteration);
    std::cout << (ends-starts) << std::endl;
    std::cout << "Size: " << read_block_size << "Bandwidth is " << bandwidth << "MB/s" << std::endl;
    std::cout << "Size: " << read_block_size << "Dummy latency is " << latency << "ns" << std::endl;
    return 0;
}
