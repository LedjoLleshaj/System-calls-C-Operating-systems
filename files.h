/// @file files.h
/// @brief Contains FILE structs and functions.
#pragma once

/**
 * Node of a linked list of filePath.
 */
typedef struct files_list
{
    /// filepath
    char *path;
    /// next node
    struct files_list *next;
} files_list;

/**
 * @brief Displays the contents of the linked list of filePath.
 *
 * @param head pointer to the head of the linked list
 */
void print_list(files_list *head);

/**
 * @brief Concatenate the directory folder to the path path.
 *
 * @param path Root path to add directories to
 * @param directory Folder name to add to the path
 * @return size_t Char direcotry size
 */
size_t append2Path(char *path, char *directory);

/**
 * @brief Check file properties.
 *
 * @param filename
 * @param sendme
 * @param out
 * @return true is the filename starts with "_sendme" and !contains "_out"
 * @return false otherwise
 */
bool StartsWith_EndsWith(const char *filename, const char *sendme, const char *out);

/**
 * @brief Append a new node to the linked list.
 *
 * @param head pointer to the head of the linked list
 * @param path filepath to add to the linked list
 *
 * @return files_list* pointer to the head of the linked list
 *
 */
files_list *append(files_list *head, char *path);

/**
 * @brief Frees the HEAP memory occupied by the filePath list
 *
 * @param head pointer to the head of the linked list
 */
void free_list(files_list *head);

/**
 * @brief Counts the number of files in the linked list of filePath
 *
 * @param head pointer to the head of the linked list
 * @return int Numbers of file
 */
int count_files(files_list *head);

/**
 * @brief Returns the size in bytes of the file with path filePath.
 *
 * @param filePath filepath to the file
 * @return long file size in bytes
 *
 */
long getFileSize(char *filePath);

/**
 * @brief Returns 1 if the path has the size <= 4 KB, 0 otherwise.
 *
 * @param filePath filepath to the file
 * @return int returns 1 if the path has the size <= 4 KB, 0 otherwise.
 */
int checkFileSize(char *filePath);

/**
 * @brief Returns 1 if the filename begins with "sendme_" and! Contains "_out", 0 otherwise.
 *
 * @param fileName  filepath to the file
 * @return int returns 1 if the filename begins with "sendme_" and! Contains "_out", 0 otherwise.
 */
int checkFileName(char *fileName);

/**
 * Search recursively in the searchPath path for files starting with "sendme_" and! Contains "_out"
 * and which have the size <= 4 KB.
 *
 * @param searchPath path to search in
 * @param head pointer to the head of the linked list
 * @return files_list* pointer to the head of the linked list whose elements have the neccessary properties
 *
 */
files_list *find_sendme_files(char *searchPath, files_list *head);
