/**
  * @file    DataManager.h
  * @version 0.5.0
  * @author  Rafaella Neofytou, Adam Mitchell
  * @brief   Header file of the DataManager. Provides a very lightweight filesystem to facilitate the
  *          storage of arbitrary file types
  */

/** Define to prevent recursive inclusion
 */
#pragma once

/** Used to include/exclude specific utility functions and parameters;
 *  set to true to include or false to exclude
 */
#define DM_DBG true

/** Includes 
 */
#include <mbed.h>
#include "DataManager_FileSystem.h"

/** Include specific drivers dependent on target */
#if BOARD == DEVELOPMENT_BOARD_V1_1_0 || BOARD == WRIGHT_V1_0_0 || BOARD == EARHART_V1_0_0
    #include "STM24256.h"
    #define NUM_OF_WRITE_RETRIES       3

    #define PAGES                      500
    #define PAGE_SIZE_BYTES            64
    #define EEPROM_SIZE_BYTES          32000
    #define GLOBAL_STATS_START_ADDRESS 0
    #define GLOBAL_STATS_LENGTH        8
    #define FILE_TABLE_PAGES           4
    #define FILE_TABLE_START_ADDRESS   GLOBAL_STATS_LENGTH
    #define FILE_TABLE_LENGTH          ((PAGE_SIZE_BYTES * FILE_TABLE_PAGES) - GLOBAL_STATS_LENGTH)
    #define STORAGE_START_ADDRESS      FILE_TABLE_LENGTH + GLOBAL_STATS_LENGTH
    #define STORAGE_LENGTH             ((PAGES * PAGE_SIZE_BYTES) - (STORAGE_START_ADDRESS))
#endif /* #if BOARD == ... */

/** Base class for the Data Manager
 */ 
class DataManager
{

	public:

        enum
        {
            DATA_MANAGER_OK = 0
        };

        #if BOARD == DEVELOPMENT_BOARD_V1_1_0 || BOARD == WRIGHT_V1_0_0 || BOARD == EARHART_V1_0_0
        DataManager(PinName write_control, PinName sda, PinName scl, int frequency_hz);
        #endif /* #if BOARD == ... */

		~DataManager();

        /** Initialise the file table to all zeros, set file system initialised flag 
         *  and set g_stats next available address and space remaining 
         *
         * @return Indicates success or failure reason
         */
        int init_filesystem();

        /** Determine whether or not the filesystem has been initialised
         *
         * @param &initialised Address of boolean value to which result of 
         *                     an initialisation check is stored. True on 
         *                     initialised, else false
         * @return Indicates success or failure reason
         */
        int is_initialised(bool &initialised);

        /** Get global next address and space remaining counters
         *
         * @param *data Byte array to which to write global stats counters
         * @return Indicates success or failure reason
         */
        int get_global_stats(char *data);

        /** Return maximum number of files that can be stored
         *  in persistent storage
         *
         *  @return Maximum number of files that can be stored
         */
		uint16_t get_max_files();

        /** Return overall total file entry storage size in bytes
         *
         *  @return Total usable space, in bytes, for file entry storage
         */
		int get_storage_size_bytes();

        /** Add new file to the file table and allocate a region of
         *  memory within which to store entries to the file
         *
         * @param file File_t object representing the file to be stored
         * @param entries_to_store Number of unique entries of this file type to be stored
         * @return Indicates success or failure reason
         */
        int add_file(DataManager_FileSystem::File_t file, uint16_t entries_to_store);

        /** Get all File_t parameters for a given filename
         *
         * @param filename ID of file to be retrieved
         * @param &file Address of File_t object in which retrieved information
         *              will be stored
         * @return Indicates success or failure reason
         */
        int get_file_by_name(uint8_t filename, DataManager_FileSystem::File_t &file);

        /** Calculate the number of valid files current stored in memory
         * @param &valid_files   Address of integer value in which number of 
         *                       detected valid files will be stored
         * @return Indicates success or failure reason                        
         */
        int total_stored_files(int &valid_files);

        /** Calculate total number of spaces available in the file table
         *  for new entries
         *
         * @param &remaining_files Address of integer value in which the total number
         *                         of spaces available in the file table is to be
         *                         written
         * @return Indicates success or failure reason
         */
        int total_remaining_file_table_entries(int &remaining_files);

        /** Read an entry, i.e. actual data such as a measurement, from a 
         *  specific index within a file
         *
         * @param filename ID of the file from which we should read
         * @param entry_index 0-indexed position of the entry to be read
         * @param *data Pointer to an array in which the read data will be stored
         * @param data_length Length of *data in bytes
         * @return Indicates success or failure reason
         */
        int read_file_entry(uint8_t filename, int entry_index, char *data, int data_length);

        /** Write an entry, i.e. actual data such as a measurement, to next 
         *  available address within the files allocated memory region
         *
         * @param filename ID of the file to which we should append data
         * @param *data Actual data to be written to file
         * @param data_length Length of *data in bytes
         * @return Indicates success or failure reason
         */
        int append_file_entry(uint8_t filename, char *data, int data_length);

        /** By resetting the next available address to the file start address
         *  we essentially 'delete' all entries within the file whilst 
         *  retaining the actual data until it is overwritten
         *
         * @param filename ID of the file whose entries are to be cleared
         * @return Indicates success or failure reason
         */  
        int delete_file_entries(uint8_t filename);

        /** Write an entry, i.e. actual data such as a measurement, to the 
         *  first address within the files allocated memory region
         *
         * @param filename ID of the file to which we should write data
         * @param *data Actual data to be written to file
         * @param data_length Length of *data in bytes
         * @return Indicates success or failure reason
         */
        int overwrite_file_entries(uint8_t filename, char *data, int data_length);

        /** Remove entries_to_remove entries starting from index 0, shift
         *  the remaining entries to the start of the file entry table and 
         *  set the next available address to the lowest available address. 
         *  This frees up space at the end of the file entry table by removing
         *  the most historic data
         *
         * @param filename ID of the file on which this operation is to be performed
         * @param entries_to_remove Number of entries to be truncated from the 
         *                          start of the file entry table
         * @return Indicates success or failure reason
         */
        int truncate_file(uint8_t filename, int entries_to_remove);

        /** Calculate number of entries within a file
         *
         * @param filename ID of the file to be queried
         * @param &written_entries Address of integer value to which the number
         *                         of written entries should be stored
         * @return Indicates success or failure reason
         */
        int get_total_written_file_entries(uint8_t filename, int &written_entries);

        /** Calculate number of measurements that can be stored
         *
         * @param filename ID of the file to be queried
         * @param &remaining_entries Address of integer value to which the number
         *                           of remaining measurements should be stored
         * @return Indicates success or failure reason
         */
        int get_remaining_file_entries(uint8_t filename, int &remaining_entries);

        /** Calculate remaining space for entries in bytes
         *
         * @param filename ID of the file to be queried
         * @param &remaining_entries Address of integer value to which the amount
         *                           of remaining space should be stored
         * @return Indicates success or failure reason
         */
        int get_remaining_file_entries_bytes(uint8_t filename, int &remaining_bytes);

        #if DM_DBG == true
        /** Utility function to print a File_t over UART
         *
         * file The File_t object whose parameters we wish to print
         */
        void print_file(DataManager_FileSystem::File_t file);

        /** Utility function to print GlobalStats_t over UART
         *
         * g_stats The GlobalStats_t object whose parameters we wish to print
         */
        void print_global_stats(DataManager_FileSystem::GlobalStats_t g_stats);
        #endif // #if DM_DBG == true

    private:

        /** Set global next address and space remaining counters
         *
         * @param data Byte array containing data to write to global stats counters
         * @return Indicates success or failure reason
         */
        int set_global_stats(char *data);  

        /** Perform checksum on given File_t using the 'valid' parameter
         *
         * @param type File to be checked for validity
         * @return True if file is valid, else false
         */
        bool is_valid_file(DataManager_FileSystem::File_t file); 

        /** Determine the next available address to which to write file
         *
         * @param &next_available_address Address of integer value in which the address
         *                                of the next available location in memory to which
         *                                you can write a file is stored. -1 if 
         *                                there are no available spaces
         * @return Indicates success or failure reason
         */
        int get_next_available_file_table_address(int &next_available_address);

        /** Modify a file's metadata
         *
         * @param filename ID of file to be modified
         * @param file Updated version of file
         * @return Indicates success or failure reason
         */
        int modify_file(uint8_t filename, DataManager_FileSystem::File_t file);  

        #if BOARD == DEVELOPMENT_BOARD_V1_1_0 || BOARD == WRIGHT_V1_0_0 || BOARD == EARHART_V1_0_0
        STM24256 _storage;
        #endif /* #if BOARD == ... */

};