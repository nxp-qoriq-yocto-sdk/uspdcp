#include "fsl_sec.h"

void job_ring_callback(
		sec_job_ring_t	*jr_handle,
		sec_job_ring_event_t   jr_event,
		int jr_status)
		{
		return;

		}

int main(void)
{
   int job_rings_no = 2;
   sec_job_ring_t * job_ring_handles;
   
   sec_init(job_rings_no, job_ring_callback, &job_ring_handles);

   sec_release();
   return 1;
}
