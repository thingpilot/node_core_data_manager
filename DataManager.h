/**
  * @file    DataManager.h
  * @version 0.1.0
  * @author  Rafaella Neofytou, Adam Mitchell
  * @brief   Header file of the DataManager. Provides a very lightweight filesystem to facilitate the
  *          storage of arbritrary file types
  */

/** Define to prevent recursive inclusion
 */
#pragma once

/** Includes 
 */
#include <mbed.h>
#include "board.h"

#if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0)

#include "STM24256.h"

#define PAGES 500
#define PAGE_SIZE_BYTES 64
#define EEPROM_SIZE_BYTES 32000

#define TYPE_STORE_PAGES 2

#define GLOBAL_STATS_START_ADDRESS 0
#define GLOBAL_STATS_LENGTH 4
#define TYPE_STORE_START_ADDRESS GLOBAL_STATS_LENGTH
#define TYPE_STORE_LENGTH ((PAGE_SIZE_BYTES * TYPE_STORE_PAGES) - GLOBAL_STATS_LENGTH)
#define STORAGE_START_ADDRESS TYPE_STORE_LENGTH + GLOBAL_STATS_LENGTH
#define STORAGE_LENGTH ((PAGES * PAGE_SIZE_BYTES) - (STORAGE_START_ADDRESS))

namespace DataManager_FileSystem
{
    union GlobalStats_t
    {
        struct
        {
            uint16_t next_available_address;
            uint16_t space_remaining;
        } parameters;

        char data[sizeof(GlobalStats_t::parameters)];
    };

	union FileType_t
    {
        struct 
        {
            uint16_t length_bytes;
            uint16_t file_start_address;
            uint16_t file_end_address;
            uint16_t next_available_address;
            uint8_t type_id; //filename
            uint8_t valid;
        } parameters;

        char data[sizeof(FileType_t::parameters)];
    };
}
#endif /* #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0) */


/** Base class for the Data Manager
 */ 
class DataManager
{

	public:

        enum
        {
            DATA_MANAGER_OK                  = 0,
            FILE_TYPE_TABLE_FULL             = 20,
            FILE_RECORD_TABLE_FULL           = 21,
            DATA_MANAGER_INVALID_TYPE        = 22,
            FILE_TYPE_INSUFFICIENT_SPACE     = 23,
            FILE_TYPE_LENGTH_MISMATCH        = 24,
            FILE_CONTENTS_INSUFFICIENT_SPACE = 25,
            FILE_ENTRY_INVALID               = 26
        };

        #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0)
        DataManager(PinName write_control, PinName sda, PinName scl, int frequency_hz);
        #endif /* #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0) */

		~DataManager();

        /** Return maximum number of file type definitions that can be stored
         *  in persistent storage
         *
         *  @return Maximum number of file type definitions that can be stored
         */
		uint16_t get_max_types();

        /** Return overall total file storage size in bytes
         *
         *  @return Total usable space, in bytes, for file storage
         */
		int get_storage_size_bytes();

        /** Initialise the file type and record tables to all zeros
         *
         * @return Indicates success or failure reason
         */
        int init_filesystem();

        /** Get all FileType_t parameters for a given type_id
         *
         * @param type_id ID of file type definition to be retrieved
         * @param &type Address of FileType_t object in which retrieved information
         *              will be stored
         * @return Indicates success or failure reason
         */
        int get_file_type_by_id(uint8_t type_id, DataManager_FileSystem::FileType_t &type);

        /** Add new file type entry to the file type table and allocate a region of
         *  memory to store file data within
         *
         * @param type FileType_t object representing file type definition to be
         *             stored into persistent storage medium
         * @param quantity_to_store Number of unique entries of this file type to be stored
         * @return Indicates success or failure reason
         */
        int add_file_type(DataManager_FileSystem::FileType_t type, uint16_t quantity_to_store);

        /** Calculate the number of valid file type definitions currently 
         *  stored in memory
         * @param &valid_entries Address of integer value in which number of 
         *                       detected valid entries will be stored
         * @return Indicates success or failure reason                        
         */
        int total_stored_file_type_entries(int &valid_entries);

        /** Calculate total number of spaces available in the file type definition table
         *  for new entries
         *
         * @param &remaining_entries Address of integer value in which the total number
         *                           of spaces available in the file type table is to be
         *                           written
         * @return Indicates success or failure reason
         */
        int total_remaining_file_type_entries(int &remaining_entries);

        /** Determine the next available address to which to write file type definition
         *
         * @param &next_available_address Address of integer value in which the address
         *                                of the next available location in memory to which
         *                                you can write a file type entry is stored. -1 if 
         *                                there are no available spaces
         * @return Indicates success or failure reason
         */
        int get_next_available_file_type_table_address(int &next_available_address);

        /** Get global next address and space remaining counters
         *
         * @param data Byte array to which to write global stats counters
         * @return Indicates success or failure reason
         */
        int get_global_stats(char *data);

        /** Read actual data, i.e. a measurement, from a specific index within a file
         *
         * @param type_id ID of the file from which we should read
         * @param entry_index 0-indexed position of the entry to be read
         * @param *data Pointer to an array in which the read data will be stored
         * @param data_length Length of *data in bytes
         * @return Indicates success or failure reason
         */
        int read_file_entry(uint8_t type_id, int entry_index, char *data, int data_length);

        /** Write actual data, i.e. a measurement, to next available address
         *  within the files allocated memory region
         *
         * @param type_id ID of the file to which we should append data
         * @param *data Actual data to be written to file
         * @param data_length Length of *data in bytes
         * @return Indicates success or failure reason
         */
        int append_to_file(uint8_t type_id, char *data, int data_length);

        /** By resetting the next available address to the file start address
         *  we essentially 'delete' all contents of the file while retaining the
         *  actual data until it is overwritten
         *
         * @param type_id ID of the file type definition whose contents are to be cleared
         * @return Indicates success or failure reason
         */  
        int delete_file_contents(uint8_t type_id);

        /** Write actual data, i.e. a measurement, to the first address
         *  within the files allocated memory region
         *
         * @param type_id ID of the file to which we should append data
         * @param *data Actual data to be written to file
         * @param data_length Length of *data in bytes
         * @return Indicates success or failure reason
         */
        int overwrite_file(uint8_t type_id, char *data, int data_length);

        int truncate_file(uint8_t type_id, int entries_to_truncate);

        /** Calculate number of entries within a file
         *
         * @param type_id ID of the file to be queried
         * @param &written_entries Address of integer value to which the number
         *                         of written entries should be stored
         * @return Indicates success or failure reason
         */
        int get_total_written_file_entries(uint8_t type_id, int &written_entries);

        /** Calculate number of measurements that can be stored
         *
         * @param type_id ID of the file to be queried
         * @param &remaining_entries Address of integer value to which the number
         *                           of remaining measurements should be stored
         * @return Indicates success or failure reason
         */
        int get_remaining_file_entries(uint8_t type_id, int &remaining_entries);

        /** Calculate remaining space for measurement in bytes
         *
         * @param type_id ID of the file to be queried
         * @param &remaining_entries Address of integer value to which the amount
         *                           of remaining space should be stored
         * @return Indicates success or failure reason
         */
        int get_remaining_file_size(uint8_t type_id, int &remaining_bytes);

    private:

        /** Perform checksum on given FileType_t using the 'valid' parameter
         *
         * @param type File type defintion to be checked for validity
         * @return True if file type entry is valid, else false
         */
        bool is_valid_file_type(DataManager_FileSystem::FileType_t type);

        /** Set global next address and space remaining counters
         *
         * @param data Byte array containing data to write to global stats counters
         * @return Indicates success or failure reason
         */
        int set_global_stats(char *data);   

        /** Modify file type definition
         *
         * @param type_id ID of file type definition to be modified
         * @param type Updated version of file type definition
         * @return Indicates success or failure reason
         */
        int modify_file_type(uint8_t type_id, DataManager_FileSystem::FileType_t type);  

        #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0)
        STM24256 _storage;
        #endif /* #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0) */

};