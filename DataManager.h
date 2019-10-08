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
#define TYPE_STORE_PAGES 2
#define RECORD_STORE_PAGES 125

#define TYPE_STORE_START_ADDRESS 0
#define TYPE_STORE_LENGTH PAGE_SIZE_BYTES * TYPE_STORE_PAGES
#define RECORD_STORE_START_ADDRESS TYPE_STORE_LENGTH
#define RECORD_STORE_LENGTH PAGE_SIZE_BYTES * RECORD_STORE_PAGES
#define STORAGE_START_ADDRESS TYPE_STORE_LENGTH + RECORD_STORE_LENGTH
#define STORAGE_LENGTH (PAGES * PAGE_SIZE_BYTES) - (STORAGE_START_ADDRESS)

namespace DataManager_FileSystem
{
	union FileType_t
    {
        struct 
        {
            uint16_t length_bytes;
            uint8_t type_id;
            uint8_t valid;
        } parameters;

        char data[sizeof(FileType_t::parameters)];
    };

	union FileRecord_t
	{
		struct
		{
			uint16_t start_address;
			uint16_t length_bytes;
			uint16_t record_id;
			uint8_t type_id;
			uint8_t valid;
		} parameters;
		
		char data[sizeof(FileRecord_t::parameters)];
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
            DATA_MANAGER_OK      = 0,
            FILE_TYPE_TABLE_FULL = 1
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

        /** Return maximum number of file records that can be stored
         *  in persistent storage
         *
         *  @return Maximum number of file records that can be stored
         */
		int get_max_records();

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

        /** Add new file type entry to the file type table
         *
         * @param type_id Enumerated value of the file type to be added
         * @param length_bytes Size of the sum of the struct's components,
         *                     equivalent to sizeof(yourStruct)
         * @return Indicates success or failure reason
         */
        int add_file_type(uint8_t type_id, uint16_t length_bytes);

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

    private:

        /** Perform checksum on given FileType_t using the 'valid' parameter
         *
         * @param type File type defintion to be checked for validity
         * @return True if file type entry is valid, else false
         */
        bool is_valid_file_type(DataManager_FileSystem::FileType_t type);

        #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0)
        STM24256 _storage;
        #endif /* #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0) */

};