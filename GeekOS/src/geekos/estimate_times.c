#include <geekos/kthread.s>

struct {
	int cylinder;
	int track;
	
	/* Disk characteristics */
	int block_size,
	int tot_blocks;
	int tot_tracks;
	int track_cap;
} disk_state;

int estimateTime(int no_of_bytes) {
	int cyl_shift = time_to_shift_tracsk
}