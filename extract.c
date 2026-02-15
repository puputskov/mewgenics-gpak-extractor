#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <Windows.h>

typedef struct buffer_t
{
	uint64_t size;
	uint64_t offset;
	uint8_t  *data;
} buffer_t;

typedef struct gpak_file_info_t
{
	char *filename;
	uint32_t file_size;
	uint16_t filename_length;
} gpak_file_info_t;


void print_if_error (int32_t result, const char *fmt, ...)
{
	if (result != EXIT_SUCCESS)
	{
		va_list args;
		va_start (args, fmt);
		vprintf (fmt, args);
		va_end (args);
	}
}

uint64_t get_file_size (HANDLE file)
{
	uint32_t high = 0;
	uint32_t low = GetFileSize (file, &high);

	uint64_t file_size = ((uint64_t) low) | (((uint64_t) high) << 32ULL);
	return (file_size);
}

BOOL write_to_file (HANDLE file, const uint8_t *src, uint64_t size)
{
	BOOL ok = TRUE;
	uint64_t total_write = 0;
	while (total_write < size && TRUE == ok)
	{
		uint64_t left = (size - total_write);
		uint32_t bytes_to_write = (left >= 0xFFFFFFFFU) ? 0xFFFFFFFF : left;
		uint32_t write = 0;
		ok = WriteFile (file, src + total_write, bytes_to_write, &write, NULL);
		total_write += write;
	}

	return (ok);
}

BOOL read_from_file (HANDLE file, uint8_t *dest, uint64_t size)
{
	BOOL ok = TRUE;
	uint64_t total_read = 0;
	while (total_read < size && TRUE == ok)
	{
		uint64_t left = (size - total_read);
		uint32_t bytes_to_read = (left >= 0xFFFFFFFFU) ? 0xFFFFFFFF : left;
		uint32_t read = 0;
		ok = ReadFile (file, dest + total_read, bytes_to_read, &read, NULL);
		total_read += read;
	}

	return (ok);
}

BOOL write_to_buffer (buffer_t *buffer, const void *data, uint64_t size)
{	
	BOOL result = TRUE;
	uint64_t new_offset = 0;

	if (NULL == buffer || NULL == data)
	{
		result = FALSE;
	}

	
	if (TRUE == result)
	{
		new_offset = buffer->offset + size;
		result = (new_offset <= buffer->size);
	}


	if (TRUE == result)
	{
		memcpy (buffer->data + buffer->offset, data, size);
		buffer->offset = new_offset;
	}

	return (result);
}

BOOL read_from_buffer (buffer_t *buffer, void *dest, uint64_t size, void **ptr)
{
	BOOL result = TRUE;
	uint64_t new_offset = 0;

	if (NULL == buffer)
	{
		result = FALSE;
	}

	
	if (TRUE == result)
	{
		new_offset = buffer->offset + size;
		result = (new_offset <= buffer->size);
	}


	if (TRUE == result)
	{
		if (dest != NULL)
		{
			memcpy (dest, buffer->data + buffer->offset, size);
		}

		if (NULL != ptr)
		{
			*ptr = buffer->data + buffer->offset;
		}

		buffer->offset = new_offset;
	}

	return (result);
}


uint64_t find_first (const char *str, uint64_t length, char find)
{
	uint64_t offset = ~0ULL;
	for (uint64_t i = 0; i < length && (~0ULL) == offset; ++ i)
	{
		if (str [i] == find)
		{
			offset = i;
		}
	}

	return (offset);
}

uint64_t find_last (const char *str, uint64_t length, char find)
{
	uint64_t offset = ~0ULL;
	for (uint64_t i = 0; i < length; ++ i)
	{
		if (str [i] == find)
		{
			offset = i;
		}
	}

	return (offset);
}

void create_folder_structure_from_filepath (
	const char *filepath,
	uint64_t length
)
{

	uint64_t first = find_first (filepath, length, '/');
	while (length > first)
	{
		uint8_t buffer_data [1024] = {0}; // Lazy...
		buffer_t buffer = {
			.size = 1024,
			.offset = 0,
			.data = buffer_data
		};
		
		write_to_buffer (&buffer, filepath, first);
		CreateDirectoryA (buffer_data, NULL); // @TODO(Jyri): Check for errors
		++ first;
		uint64_t next = find_first (filepath + first, length, '/');	
		
		if ((~0ULL) != next)
		{
			first += next;
		}
	}
}


int main (int argc, const char **argv)
{
	int32_t result = EXIT_SUCCESS;
	buffer_t buffer = {0};
	
	uint32_t num_files = 0;
	gpak_file_info_t *file_info = NULL;
	
	if (argc != 2)
	{
		printf ("Usage: mew.exe [path]\n");
		result = EXIT_FAILURE;
	}


	HANDLE file = INVALID_HANDLE_VALUE;
	if (EXIT_SUCCESS == result)
	{
		file = CreateFileA (
			argv [1],
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		result = ((INVALID_HANDLE_VALUE != file) ? result : EXIT_FAILURE);
		print_if_error (result, "Error: failed to open file: %s\n", argv [1]);
	}


	if (EXIT_SUCCESS == result)
	{
		uint64_t file_size = get_file_size (file);
		buffer = (buffer_t) {
			.size = file_size,
			.offset = 0,
			.data = malloc (file_size)
		};

		result = ((NULL != buffer.data) ? result : EXIT_FAILURE);
		print_if_error (
			result,
			"Failed to allocate %llu bytes to memory!",
			file_size
		);
	}

	if (EXIT_SUCCESS == result)
	{
		BOOL ok = read_from_file (file, buffer.data, buffer.size);
		result = (TRUE == ok) ? result : EXIT_FAILURE;
		print_if_error (result, "Failed to read file to memory!\n");
	}

	if (EXIT_SUCCESS == result)
	{	
		read_from_buffer (&buffer, &num_files, 4, NULL);
		result = (0 < num_files) ? result : EXIT_FAILURE;
		print_if_error (result, "There are no files...\n");
	}


	if (EXIT_SUCCESS == result)
	{
		file_info = (gpak_file_info_t *) malloc (
			sizeof (gpak_file_info_t) * num_files
		);

		result = (NULL != file_info) ? result : EXIT_FAILURE;
		print_if_error (
			result,
			"Failed to allocate file info for %u files!\n",
			num_files
		);
	}


	if (EXIT_SUCCESS == result)
	{
		BOOL ok = TRUE;		
		uint64_t total_size_of_files = 0;
		for (uint32_t i = 0; i < num_files && ok; ++ i)
		{
			uint16_t filename_len = 0;
			ok = ok && read_from_buffer (
				&buffer,
				&file_info [i].filename_length,
				2,
				NULL
			);

			ok = ok && read_from_buffer (
				&buffer,
				NULL,
				file_info [i].filename_length,
				&file_info [i].filename
			);

			ok = ok && read_from_buffer (
				&buffer,
				&file_info [i].file_size,
				4,
				NULL
			);

			total_size_of_files += file_info [i].file_size;
		}
	
		result = (TRUE == ok) ? result : EXIT_FAILURE;
		print_if_error (result, "Not enough data to read the file info\n");

		uint64_t left = (buffer.size - buffer.offset);
		result = (left >= total_size_of_files) ? result : EXIT_FAILURE;
		print_if_error (
			result,
			"Total size of all files is larger than the amount of data left!\n"
		);
	}

	if (EXIT_SUCCESS == result)
	{
		BOOL ok = TRUE;
		for (uint32_t i = 0; i < num_files && ok; ++ i)
		{
			uint8_t buffer_data [1024] = {0}; // Lazy...
			buffer_t filename_buffer = {
				.size = 1024,
				.offset = 0,
				.data = buffer_data
			};

			
			write_to_buffer (&filename_buffer, "extracted/", 10);
			

			uint64_t path_length = find_last (
				file_info [i].filename,
				file_info [i].filename_length,
				'/'
			);

			if ((~0ULL) != path_length)
			{
				write_to_buffer (
					&filename_buffer,
					file_info [i].filename,
					++ path_length
				);
			}

			else
			{
				path_length = 0;
			}

			create_folder_structure_from_filepath (
				filename_buffer.data,
				filename_buffer.offset
			);

			write_to_buffer (
				&filename_buffer,
				file_info [i].filename + path_length,
				file_info [i].filename_length - path_length
			);

			printf ("file: %s: ", filename_buffer.data);
			HANDLE f = CreateFileA (
				filename_buffer.data,
				GENERIC_WRITE,
				0,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);

			if (INVALID_HANDLE_VALUE != f)
			{
				// @TODO(Jyri): check for possible errors etc....
				void *ptr = NULL;
				ok = ok && read_from_buffer (
					&buffer,
					NULL,
					file_info [i].file_size,
					&ptr
				);

				write_to_file (f, ptr, file_info [i].file_size);
				printf ("Ok\n");
				CloseHandle (f);
			}
			else
			{
				result = EXIT_FAILURE;
				printf ("Failed!\n");
			}
		}
	}

	

	// Clean up, kind of pointless...
	if (INVALID_HANDLE_VALUE != file)
	{
		CloseHandle (file);
	}

	
	if (NULL != file_info)
	{
		free (file_info);
	}

	if (NULL != buffer.data)
	{
		free (buffer.data);
	}
	return (result);
}

