#include "math.h"
#include "miner.h"
#include "usbutils.h"

enum miner_state {
	MINER_INIT = 1,
	MINER_CHIP_COUNT,
	MINER_CHIP_COUNT_XX,
	MINER_CHIP_COUNT_OK,
	MINER_OPEN_CORE,
	MINER_OPEN_CORE_OK,
	MINER_MINING,
	MINER_MINING_DUPS,
	MINER_SHUTDOWN,
	MINER_SHUTDOWN_OK,
	MINER_RESET
};

enum miner_asic {
	BM1384 = 1,
	BM1387
};

enum micro_command {
	M1_GET_FAN    = (0x00 << 3),
	M1_GET_RPM    = (0x01 << 3),
	M1_GET_VIN    = (0x02 << 3),
	M1_GET_IIN    = (0x03 << 3),
	M1_GET_TEMP   = (0x04 << 3),
	M1_GET_VNODE0 = (0x05 << 3),

	M1_CLR_BEN    = (0x08 << 3),
	M1_SET_BEN    = (0x09 << 3),
	M1_CLR_LED    = (0x0A << 3),
	M1_SET_LED    = (0x0B << 3),
	M1_CLR_RST    = (0x0C << 3),
	M1_SET_RST    = (0x0D << 3),

	M2_SET_FAN    = (0x18 << 3),
	M2_SET_VCORE  = (0x1C << 3)
};

struct COMPAC_INFO {

	enum sub_ident ident;            // Miner identity
	enum miner_state mining_state;   // Miner state
	enum miner_asic asic_type;       // ASIC Type
	struct thr_info *thr;            // Running Thread
	struct thr_info rthr;            // Listening Thread
	struct thr_info xthr;            // Unplug Monitor Thread

	pthread_mutex_t lock;        // Mutex
	pthread_mutex_t wlock;       // Mutex Serialize Writes

	float frequency;             // Chip Frequency
	float frequency_requested;   // Requested Frequency
	float frequency_start;       // Starting Frequency

	float micro_temp;            // Micro Reported Temp

	uint32_t scanhash_ms;        // Avg time(ms) inside scanhash loop
	uint32_t task_ms;            // Avg time(ms) between task sent to device
	uint32_t fullscan_ms;        // Estimated time(ms) for full nonce range
	uint64_t hashrate;           // Estimated hashrate = cores x chips x frequency

	uint64_t task_hcn;           // Hash Count Number - max nonce iter.
	uint32_t prev_nonce;         // Last nonce found

	int failing;                 // Flag failing sticks
	int fail_count;              // Track failures.
	int accepted;                // Nonces accepted
	int dups;                    // Duplicates found
	int interface;               // USB interface
	int nonceless;               // Tasks sent.  Resets when nonce is found.
	int nonces;                  // Nonces found
	int zero_check;              // Received nonces from zero work
	int vcore;                   // Core voltage
	int micro_found;             // Found a micro to communicate with

	uint32_t bauddiv;            // Baudrate divider
	uint32_t chips;              // Stores number of chips found
	uint32_t cores;              // Stores number of core per chp
	uint32_t difficulty;         // For computing hashrate
	uint64_t hashes;             // Hashes completed
	uint32_t job_id;             // JobId incrementer
	uint32_t low_hash;           // Tracks of low hashrate
	uint32_t max_job_id;         // JobId cap
	uint32_t ramping;            // Ramping incrementer
	uint32_t rx_len;             // rx length
	uint32_t task_len;           // task length
	uint32_t ticket_mask;        // Used to reduce flashes per second
	uint32_t tx_len;             // tx length
	uint32_t update_work;        // Notification of work update

	struct timeval start_time;              // Device startup time
	struct timeval last_scanhash;           // Last time inside scanhash loop
	struct timeval last_reset;              // Last time reset was triggered
	struct timeval last_task;               // Last time work was sent
	struct timeval last_nonce;              // Last time nonce was found
	struct timeval last_hwerror;            // Last time hw error was detected
	struct timeval last_frequency_adjust;   // Last time of frequency adjust
	struct timeval last_frequency_ping;     // Last time of frequency poll
	struct timeval last_frequency_report;   // Last change of frequency report
	struct timeval last_chain_inactive;     // Last sent chain inactive
	struct timeval last_micro_ping;         // Last time of micro controller poll
	struct timeval last_write_error;        // Last usb write error message

	unsigned char task[64];                 // Task transmit buffer
	unsigned char cmd[32];                  // Command transmit buffer
	unsigned char rx[32];                   // Receive buffer
	unsigned char tx[32];                   // Transmit buffer

	struct work *work[0x7F];                // Work ring buffer

};

void stuff_lsb(unsigned char *dst, uint32_t x);
void stuff_msb(unsigned char *dst, uint32_t x);
void stuff_reverse(unsigned char *dst, unsigned char *src, uint32_t len);
uint64_t bound(uint64_t value, uint64_t lower_bound, uint64_t upper_bound);
