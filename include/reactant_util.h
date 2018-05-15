/*******************************************************************************
 * Author:  Daniel J. Stotts
 * Purpose: Includes multiple utilities for use with the Reactant project
 * Revision Date: 1/9/2018
 ******************************************************************************/

#ifndef REACTANT_UTIL_H
#define REACTANT_UTIL_H

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <exsrc_aes.h>


/*******************************************************************************
 *  Category:   General
 *  Description:    Defines elements used by other categories
 ******************************************************************************/
// Constant definitions
#define BUFFER_DEPTH (256)  // Buffer depth

// Macro definitions
#define PH(para) Placeholder_ ## para __attribute__((unused))   // Set a parameter as a placeholder, i.e. unused
                                                                // Use to match function signatures without warnings
// Control enum
typedef enum _control_t
{
    DISABLE = 0,
    ENABLE,

} control_t;

// Count digits in integer
int digits(int i, int base);

// Reverse the bits of a byte
char reverse_byte(unsigned char byte);

/*******************************************************************************
 *  Category:   Error checking
 *  Description:    Allows function return status to be easily inspected
 ******************************************************************************/
// Status
extern int reactant_errno; // TODO: Refactor to return intuitively

extern char * _general_status_message[];
typedef enum _status_t
{
    UNKNOWN = -1,   // No known status
    SUCCESS,    // No error
    ARGUMENT,   // Invalid argument

    _EI,    // (Extension Index) Final element of general status codes

} status_t;

// Error check function
void _error_check(int, int, char *[]);

// Macro to insert line number into _error_check call
#define error_check(index, message) _error_check(__LINE__, index, message)  // Print status of function call


/*******************************************************************************
*   Category:   Debug utilities
*   Description:    Aids in the debugging of a program
*******************************************************************************/
// Debug functions
void debug_control(control_t);
void debug_output(const char *, ...);


/*******************************************************************************
 *  Category:   Hash table
 *  Description:    Implements a simple, generic hash table "class"
 ******************************************************************************/
// Hash table data type
typedef struct _hash_data_t
{
    void * key;
    void * value;

    struct _hash_data_t * _next;

} hash_data_t;

// Hash table object type
typedef struct _hash_table_t
{
    uint32_t (*_hash)(void *);
    uint8_t (*_compare)(void *, void *);

    uint32_t size;

    uint32_t _key_size;
    uint32_t _value_size;
    hash_data_t ** _array;

} hash_table_t;

// Hash table status
extern char * _ht_status_message[];
#define ht_check(function) error_check(function, _ht_status_message)

typedef enum _ht_status_t
{
    HT_DNE = _EI,   // Key does not exist
    HT_DUPLICATE,   // Key already exists in hash table

} ht_status_t;

// Hash table functions
int ht_construct(hash_table_t * hash_table, uint32_t size, uint32_t key_size, uint32_t value_size, \
                 uint32_t (*hash)(void *), uint8_t (*compare)(void *, void *));
int ht_destruct(hash_table_t * hash_table);

int ht_search(hash_table_t * hash_table, hash_data_t * hash_data, void * key);
int ht_insert(hash_table_t * hash_table, void * key, void * value);
int ht_remove(hash_table_t * hash_table, void * key);
int ht_traverse(hash_table_t * hash_table, void * (*visit)(void *, void *));


/*******************************************************************************
 *  Category:   Registry queue
 *  Description:    Implements a simple queue. Intended for use with storing
 *                  pointers in a FIFO arrangement. It does not allocate memory
 *                  for an inserted element, but rather will store the pointer
 *                  to that element.
 ******************************************************************************/
// Queue object type
typedef struct _queue_t
{
    void ** queue;      // Array of items
    size_t capacity;    // Maximum size of the queue
    size_t size;        // Amount of items stored in the queue

    size_t head;    // Head item index
    size_t tail;    // Tail item index

    sem_t * enqueue_semaphore;  // Semaphore used to enable blocking enqueue
    sem_t * dequeue_semaphore;  // Semaphore used to enable blocking dequeue
    pthread_mutex_t * enqueue_mutex;    // Mutex used to enable threadsafe enqueue
    pthread_mutex_t * dequeue_mutex;    // Mutex used to enable threadsafe dequeue
    pthread_mutex_t * enqueue_blocking_mutex;   // Mutex used to enable threadsafe blocking enqueue
    pthread_mutex_t * dequeue_blocking_mutex;   // Mutex used to enable threadsafe blocking dequeue

} queue_t;

 // Queue status
extern char * _queue_status_message[];
#define queue_check(function) error_check(function, _queue_status_message)

typedef enum _queue_status_t
{
    QUEUE_FULL = _EI,   // Queue is full, cannot enqueue item
    QUEUE_EMPTY,        // Queue is empty, cannot dequeue item
    QUEUE_LOCK,         // Mutex is locked, cannot operate

} queue_status_t;

// Queue functions
int queue_construct(queue_t * queue, size_t capacity);
int queue_destruct(queue_t * queue);

int enqueue(queue_t * queue, void * item);
int enqueue_blocking(queue_t * queue, void * item);
int dequeue(queue_t * queue, void ** item);
int dequeue_blocking(queue_t * queue, void ** item);


/*******************************************************************************
 *  Category:   Message protocol
 *  Description:    Implements helper functions for using the Reactant message
 *                  protocol.
 ******************************************************************************/
// Constant definitions
#define MESSAGE_LENGTH (288)

// Macro definitions
#define CAPTURE_BYTE(i, n) (((i) & (0xFF << (8 * (n)))) >> (8 * (n)))

// Message object type
typedef struct _message_t
{
    short bytes_remaining;  // 2 bytes
    unsigned int source_id; // 4 bytes
    char payload[250];      // 250 bytes (where the last byte must always be zero)
    char hmac[32];

    char message_string[288];   // Full message, built from components

} message_t;

 // Message status
extern char * _message_status_message[];
#define message_check(function) error_check(function, _message_status_message)

typedef enum _message_status_t
{
    MESSAGE_NO_AUTH = _EI, // Message hash does not match

} message_status_t;

// Message functions
int message_initialize(message_t * message);
int message_pack(message_t * message, const char * key, const char * iv);
int message_unpack(message_t * message, const char * key, const char * iv);
int message_debug_hex(char * message);
unsigned char * message_hash(char * message);

#endif // REACTANT_UTIL_H
