/*******************************************************************************
 * Author:  Daniel J. Stotts
 * Purpose: Includes multiple utilities for use with the Reactant project
 * Revision Date: 1/9/2018
 ******************************************************************************/

#include "reactant_util.h"


// #############################################################################
// #                                                                           #
// #    Error checking                                                         #
// #                                                                           #
// #############################################################################
char * _general_status_message[] =
{
    "An unkown status was returned",
    "Function returned successfully",
    "Invalid argument",

};

/*******************************************************************************
 *  Function:   Error check
 *  Description:    Outputs error information based on a given index (should be
 *                  an enum) and a given array of messages (that should be in
 *                  parallel with the enum)
 ******************************************************************************/
void _error_check(int line, int index, char * message[])
{
    // Skip output of successful returns
    if(index != 0)
    {
        // If given a general status code
        if(index < _EI)
        {
            debug_output("%s%d%s%s\n", ">> Error on line ", line, ": ", _general_status_message[index + 1]);
        }
        // If given a category-specific status code
        else
        {
            debug_output("%s%d%s%s\n", ">> Error on line ", line, ": ", message[index - _EI]);
        }
    }
}


// #############################################################################
// #                                                                           #
// #    Debug utilities                                                        #
// #                                                                           #
// #############################################################################
static uint8_t DEBUG_VERBOSE = 0;

/*******************************************************************************
 *  Function:   Debug enable
 *  Description:    Enables or disables debug output
 ******************************************************************************/
void debug_control(control_t value)
{
    DEBUG_VERBOSE = (value > 0);
}

/*******************************************************************************
 *  Function:   Debug output
 *  Description:    Outputs text if enabled via debug_enable
 ******************************************************************************/
void debug_output(const char * format, ...)
{
    char buffer[BUFFER_DEPTH];
    va_list args;

    if(DEBUG_VERBOSE)
    {
        // Initialize variable argument list
        va_start(args, format);

        // Write formatted string into buffer
        vsnprintf(buffer, sizeof(buffer), format, args);

        // Print formatted string
        printf(buffer);
        fflush(stdout);

        // Clean up memory allocated to the variable argument list
        va_end(args);
    }
}


// #############################################################################
// #                                                                           #
// #    Hash table                                                             #
// #                                                                           #
// #############################################################################
char * _ht_status_message[] =
{
    "Key does not exist",
    "Key already exists",

};

/*******************************************************************************
 *  Function:   Hash table constructor
 *  Description:    Acts as the constructor for a hash table object, fills the
 *                  given pointer with the new hash table object
 ******************************************************************************/
int ht_construct(hash_table_t * hash_table, uint32_t size, uint32_t key_size, \
uint32_t value_size, uint32_t (*hash)(void *), uint8_t (*compare)(void *, void *))
{
    // If all given information is valid
    if(hash_table && hash && compare)
    {
        // Clear current hash table
        memset(hash_table, 0, sizeof(hash_table_t));

        // Allocate array
        hash_table->_array = calloc(size, sizeof(hash_data_t *));

        // Assign methods
        hash_table->_hash = hash;
        hash_table->_compare = compare;

        // Assign variables
        hash_table->size = size;
        hash_table->_key_size = key_size;
        hash_table->_value_size = value_size;
    }
    else
    {
        return ARGUMENT;
    }

    return SUCCESS;
}

/*******************************************************************************
 *  Function:   Hash table destructor
 *  Description:    Acts as the destructor for the given hash table object,
 *                  de-allocates all allocated memory
 ******************************************************************************/
int ht_destruct(hash_table_t * hash_table)
{
    int i;
    hash_data_t * position, * previous;

    // If given a valid pointer
    if(hash_table)
    {
        // If hash table has an array of hash data
        if(hash_table->_array)
        {
            // De-allocate all table elements
            for(i = 0; i < hash_table->size; ++i)
            {
                // If there is a hash data object at this index
                if(hash_table->_array[i])
                {
                    // Clear all data in linked list
                    // Set position to first hash data object in linked list
                    position = hash_table->_array[i];

                    // While there are still data objects to remove
                    while(position)
                    {
                        // Increment position
                        previous = position;
                        position = position->_next;

                        // De-allocate key in this object
                        if(previous->key)
                        {
                            free(previous->key);
                        }

                        // De-allocate value in this object
                        if(previous->value)
                        {
                            free(previous->value);
                        }

                        // De-allocate this object
                        free(previous);
                    }
                }
            }
            // De-allocate the array
            free(hash_table->_array);
        }
    }
    else
    {
        return ARGUMENT;
    }

    return SUCCESS;
}

/*******************************************************************************
 *  Function:   Hash table search
 *  Description:    Searches the given hash table object for a data object that
 *                  matches the specified key, and sets a pointer to that
 *                  data object if found
 ******************************************************************************/
int ht_search(hash_table_t * hash_table, hash_data_t * hash_data, void * key)
{
    uint32_t key_index;
    hash_data_t * position;
    int rval = HT_DNE;

    // If all given information is valid
    if(hash_table && hash_data && hash_table && key)
    {
        // Clear current hash data
        memset(hash_data, 0, sizeof(hash_data_t));

        // Find where this data object should be based on its key
        key_index = hash_table->_hash(key);
        position = hash_table->_array[key_index];

        // While there are still data objects at this index
        while(position)
        {
            // Check if the hash data object at the current position matches the given key
            if(hash_table->_compare(position->key, key))
            {
                // Set given struct equal to found data
                hash_data->key = position->key;
                hash_data->value = position->value;

                // Set _next pointer to null
                hash_data->_next = 0;
                rval = SUCCESS;
            }
            position = position->_next;
        }
    }
    else
    {
        rval = ARGUMENT;
    }

    return rval;
}

/*******************************************************************************
 *  Function:   Hash table insert
 *  Description:    Inserts data into the given hash table, first creating a
 *                  hash data object using the given key and value
 ******************************************************************************/
int ht_insert(hash_table_t * hash_table, void * key, void * value)
{
    uint32_t key_index;
    hash_data_t * position;
    hash_data_t search;

    // If all given information is valid
    if(hash_table && key && value)
    {
        // Ensure no duplicate keys are already in the table
        if(ht_search(hash_table, &search, key) == SUCCESS)
        {
            return HT_DUPLICATE;
        }

        // Find where this data object should go based on its key
        key_index = hash_table->_hash(key);
        position = hash_table->_array[key_index];

        // If there is nothing at this index
        if(position == 0)
        {
            hash_table->_array[key_index] = calloc(1, sizeof(hash_data_t));

            position = hash_table->_array[key_index];
        }
        else
        {
            // Find first null location in the linked list at this index
            while(position->_next)
            {
                position = position->_next;
            }

            // Allocate memory for new hash data object
            position->_next = calloc(1, sizeof(hash_data_t));

            position = position->_next;
        }

        // Allocate key and value
        position->key = calloc(1, hash_table->_key_size);
        position->value = calloc(1, hash_table->_value_size);

        // Assign key and value
        memcpy(position->key, key, hash_table->_key_size);
        memcpy(position->value, value, hash_table->_value_size);
    }
    else
    {
        return ARGUMENT;
    }

    return SUCCESS;
}

/*******************************************************************************
 *  Function:   Hash table remove
 *  Description:    Removes a hash data object from the given hash table,
 *                  specified by the given key
 ******************************************************************************/
int ht_remove(hash_table_t * hash_table, void * key)
{
    return UNKNOWN;
}

/*******************************************************************************
 *  Function:   Hash table traverse
 *  Description:    Traverses the given hash table, giving each key and value
 *                  to the given function
 ******************************************************************************/
int ht_traverse(hash_table_t * hash_table, void (*visit)(void *, void *))
{
    int i;
    hash_data_t * position;

    // If all given information is valid
    if(hash_table && visit)
    {
        for(i = 0; i < hash_table->size; ++i)
        {
            // Set position to first hash data object in linked list
            position = hash_table->_array[i];

            if(position) debug_output("%s%d\n", "Index: ", i);

            // While there are still data objects to visit
            while(position)
            {
                // Give information to visitor function
                visit(position->key, position->value);

                position = position->_next;
            }
        }
    }
    else
    {
        return ARGUMENT;
    }

    return SUCCESS;
}


// #############################################################################
// #                                                                           #
// #    Registry queue                                                         #
// #                                                                           #
// #############################################################################
char * _queue_status_message[] =
{
    "Queue is full",
    "Queue is empty",
    "Mutex could not be acquired",

};

/*******************************************************************************
 *  Function:   Create queue
 *  Description:    Allocates and returns a pointer to a new queue object
 ******************************************************************************/
int queue_construct(queue_t * queue, size_t capacity)
{
    if(queue)
    {
        queue->queue = malloc(sizeof(void *) * capacity);
        queue->capacity = capacity;
        queue->size = 0;
        queue->head = 0;
        queue->tail = 0;

        queue->enqueue_semaphore = malloc(sizeof(sem_t));
        sem_init(queue->enqueue_semaphore, 0, (int) capacity);
        queue->dequeue_semaphore = malloc(sizeof(sem_t));
        sem_init(queue->dequeue_semaphore, 0, 0);

        queue->enqueue_mutex = malloc(sizeof(pthread_mutex_t));
        queue->dequeue_mutex = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(queue->enqueue_mutex, NULL);
        pthread_mutex_init(queue->dequeue_mutex, NULL);

        queue->enqueue_blocking_mutex = malloc(sizeof(pthread_mutex_t));
        queue->dequeue_blocking_mutex = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(queue->enqueue_blocking_mutex, NULL);
        pthread_mutex_init(queue->dequeue_blocking_mutex, NULL);
    }
    else
    {
        return ARGUMENT;
    }

    return SUCCESS;
}

/*******************************************************************************
 *  Function:   Destroy queue
 *  Description:    Acts as the destructor for the given registry queue object,
 *                  de-allocates all allocated memory
 ******************************************************************************/
int queue_destruct(queue_t * queue)
{
    if (queue)
    {
        free(queue->queue);

        // Destroy mutexes
        pthread_mutex_destroy(queue->enqueue_mutex);
        pthread_mutex_destroy(queue->dequeue_mutex);
        free(queue->enqueue_mutex);
        free(queue->dequeue_mutex);

        pthread_mutex_destroy(queue->enqueue_blocking_mutex);
        pthread_mutex_destroy(queue->dequeue_blocking_mutex);
        free(queue->enqueue_blocking_mutex);
        free(queue->dequeue_blocking_mutex);

        // Destroy semaphores
        sem_destroy(queue->enqueue_semaphore);
        sem_destroy(queue->dequeue_semaphore);
        free(queue->enqueue_semaphore);
        free(queue->dequeue_semaphore);
    }
    else
    {
        return ARGUMENT;
    }

    return SUCCESS;
}

/*******************************************************************************
 *  Function:   Enqueue
 *  Description:    Adds an item to the given queue
 ******************************************************************************/
int enqueue(queue_t * queue, void * item)
{
    int rval = QUEUE_FULL;

    if (queue && item)
    {
        pthread_mutex_lock(queue->enqueue_mutex);

        // If the item can be enqueued.
        if ((queue->head != queue->tail || queue->size == 0) && sem_trywait(queue->enqueue_semaphore) == 0)
        {
            // Enqueue the item.
            queue->queue[queue->head] = item;
            queue->head = (queue->head + 1) % queue->capacity;
            queue->size += 1;
            rval = SUCCESS;

            sem_post(queue->dequeue_semaphore);
        }

        pthread_mutex_unlock(queue->enqueue_mutex);
    }
    else
    {
        rval = ARGUMENT;
    }

    return rval;
}

/*******************************************************************************
 *  Function:   Enqueue (blocking)
 *  Description:    Adds an item to the given queue, blocks until queue has
 *                  room for the new item
 ******************************************************************************/
int enqueue_blocking(queue_t * queue, void * item)
{
    if (queue && item)
    {
        // Wait until the item can be enqueued.
        pthread_mutex_lock(queue->enqueue_blocking_mutex);
        sem_wait(queue->enqueue_semaphore);

        // Enqueue the item.
        queue->queue[queue->head] = item;
        queue->head = (queue->head + 1) % queue->capacity;
        queue->size += 1;

        // Increment the dequeue semaphore.
        sem_post(queue->dequeue_semaphore);
        pthread_mutex_unlock(queue->enqueue_blocking_mutex);
    }
    else
    {
        return ARGUMENT;
    }

    return SUCCESS;
}

/*******************************************************************************
 *  Function:   Dequeue
 *  Description:    Removes an item from the given queue
 ******************************************************************************/
int dequeue(queue_t * queue, void * item)
{
    int rval = QUEUE_EMPTY;

    if (queue && item)
    {
        pthread_mutex_lock(queue->dequeue_mutex);

        // If an item can be dequeued.
        if (queue->size > 0 && sem_trywait(queue->dequeue_semaphore) == 0)
        {
            // Dequeue the item.
            memcpy(item, queue->queue[queue->tail], sizeof(void *));
            queue->tail = (queue->tail + 1) % queue->capacity;
            queue->size -= 1;
            rval = SUCCESS;

            // Decrement dequeue and increment enqueue semaphores. Should never block.
            sem_post(queue->enqueue_semaphore);
        }

        pthread_mutex_unlock(queue->dequeue_mutex);
    }
    else
    {
        rval = ARGUMENT;
    }

    return rval;
}

/*******************************************************************************
 *  Function:   Dequeue (blocking)
 *  Description:    Removes and returns an item from the given queue, blocks
 *                  until the queue has an item to remove
 ******************************************************************************/
int dequeue_blocking(queue_t * queue, void * item)
{
    int rval = QUEUE_EMPTY;

    if (queue && item)
    {
        // Wait until an item can be dequeued.
        pthread_mutex_lock(queue->dequeue_blocking_mutex);
        sem_wait(queue->dequeue_semaphore);

        // Dequeue the item.
        memcpy(item, queue->queue[queue->tail], sizeof(void *));
        queue->tail = (queue->tail + 1) % queue->capacity;
        queue->size -= 1;
        rval = SUCCESS;

        // Increment the enqueue semaphore.
        sem_post(queue->enqueue_semaphore);
        pthread_mutex_unlock(queue->dequeue_blocking_mutex);
    }
    else
    {
        rval = ARGUMENT;
    }

    return rval;
}
