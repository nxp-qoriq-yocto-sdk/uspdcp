#include "fsl_sec.h"

int main(void)
{
   int job_rings_no = 2;
   sec_job_ring_t * job_ring_handles;
   
   sec_init(job_rings_no, &job_ring_handles);

   sec_release();
   return 1;
}
