#define POP_SIZE 20
#define MAX_GEN 6
#define pc 0.85
#define pm 0.1
#define FIRST_PORT 3001
#define SERV_ADDR "127.0.0.1"
#define NUM_OF_SIZES 4
#define NUM_OF_ORDERS 3
#define numOfThreadsAtOnce  1
#define SIZE_OF_FILE 3840000
#define NAME_OF_FILE "file.wav"

const int DATA_SIZES[] = {960, 1920, 3840, 7680, 9600, 19200};
						  //5ms, 10ms, 20ms, 40ms, 50ms, 100ms

const int TIME_FOR_SIZES[] = {5, 10, 20, 40, 50, 100}; //All in milliseconds

typedef struct Scheme {
	int size;  //Size of the packet. Must be from DATA_SIZES
	int order; //Order in which the duplicates are sent
	int delayTilDuplicate; //Percentage delay(0 to 100)
} Scheme;

typedef struct TestScheme_t {
	int schemeNum;
	int portToUse;
	Scheme scheme;
} TestScheme_t;

typedef struct dupeData_t {
	Scheme scheme;
	int sockfd;
	struct sockaddr_in addr;
	int len;
} dupeData_t;


