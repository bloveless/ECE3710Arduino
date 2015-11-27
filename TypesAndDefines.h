// Define your pins here!
#define CE_pin 4
#define CSN_pin 5
#define IRQ_pin 2
#define MOSI_pin 11
#define MISO_pin 12
#define SCK_pin 13 

// Function definitions
#define PING_RETURN 1
#define PING        2

typedef struct {
  char function;
  char data1;
} functionData;

