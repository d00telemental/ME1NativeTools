#pragma once

#include <bitset>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <Windows.h>

#include "third_party/ME1-SDK/Include.h"

#include "third_party/detours/detours.h"
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "../third_party/detours/detours.lib")


// Macros.
// ============================================================

#pragma region Macros

#define ASI_SDK_CALL __declspec(noinline)
#define ASI_LOGGER_VSNPRINTF_BUFFER 1024

#pragma endregion

namespace me1asi {

	// Common I/O and logging routines.
	// ============================================================

#pragma region Common I/O and logging routines
	namespace io
	{
		/// <summary>
		/// Allocates a console window,
		/// enables output and resizes the buffer to max.
		/// </summary>
		inline void setup_console()
		{
			AllocConsole();
			freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
			freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
			freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);

			HANDLE h_console = GetStdHandle(STD_OUTPUT_HANDLE);
			CONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo;
			GetConsoleScreenBufferInfo(h_console, &lpConsoleScreenBufferInfo);

			COORD new_size = { lpConsoleScreenBufferInfo.dwSize.X, 30000 };
			SetConsoleScreenBufferSize(h_console, new_size);
			SetConsoleOutputCP(65001);

			fprintf(stdout, "stdout OK\n");
			fprintf(stderr, "stderr OK\n");
		}

		/// <summary>
		/// Frees the allocated console window.
		/// </summary>
		inline void teardown_console()
		{
			fprintf(stdout, "Bye!\n");
			FreeConsole();
		}

		/// <summary>
		/// Which output to log things to.
		/// </summary>
		enum runtime_logger_flags
		{
			LM_None      = 0 << 0,
			LM_Console   = 1 << 0,  // Log things to stdout.
			LM_File      = 1 << 1,  // Log things to an opened file handle.
			LM_NoDate    = 1 << 2,  // Do not print date.
			LM_Immediate = 1 << 3,  // Flush file output immediately.
		};

		struct runtime_logger
		{
		private:
			bool _initialized = false;
			SYSTEMTIME _local_time;

		public:
			FILE* file = NULL;

			const char* _line_format_nodate = "%s\n";
			const char* _line_format = "%02d:%02d:%02d.%03d  %s\n";

			runtime_logger()
			{
				GetLocalTime(&_local_time);
			}

			~runtime_logger()
			{
				if (file)
				{
					fclose(file);
				}
			}

			void initialize()
			{
				_initialized = true;
			}

			void initialize(const char* fname)
			{
				file = fopen(fname, "w");
				if (!file)
				{
					printf("Failed to open log file\n");
					return;
				}
				_initialized = true;
			}

			int write_line_internal(FILE* stream, bool display_date, char* line)
			{
				if (!_initialized)
				{
					printf("Writing a line while the logger is not truly initialized.\n");
					return -1;
				}

				if (!display_date)
				{
					return fprintf(stream, _line_format_nodate, line);
				}

				GetLocalTime(&_local_time);
				return fprintf(stream, _line_format, _local_time.wHour, _local_time.wMinute, _local_time.wSecond, _local_time.wMilliseconds, line);
			}

			int write_line(char* line, short flags = LM_Console | LM_Immediate)
			{
				const bool show_date = !(flags & LM_NoDate);

				if (flags & LM_Console)
				{
					write_line_internal(stdout, show_date, line);
				}

				if ((flags & LM_File) && file)
				{
					write_line_internal(file, show_date, line);
					if (flags & LM_Immediate)
					{
						fflush(file);
					}
				}

				return -1;
			}

			int write_line(const char* line, short flags = LM_Console | LM_Immediate)
			{
				return write_line(const_cast<char*>(line), flags);
			}

			int write_format_line(short flags, char* fmt_line, ...)
			{
				int rc;
				char buffer[ASI_LOGGER_VSNPRINTF_BUFFER];

				va_list args;
				va_start(args, fmt_line);
				rc = vsnprintf(buffer, ASI_LOGGER_VSNPRINTF_BUFFER, fmt_line, args);
				va_end(args);

				if (rc < 0)
				{
					write_line("write_format_line: vsnprintf encoding error");
					return -1;
				}
				else if (rc == 0)
				{
					write_line("write_format_line: vsnprintf runtime constraint violation");
					return -1;
				}

				return write_line(buffer, flags);
			}

			/// <summary>
			/// Prints bytes from a buffer in a hexdump kind of style.
			/// </summary>
			/// <param name="buf">Binary buffer.</param>
			/// <param name="len">Number of bytes.</param>
			static void write_hexdump(const unsigned char* buf, int32_t len)
			{
				const int hexdump_line_len = 16;

				unsigned char c = '\0';
				unsigned char l = '\0';

				int line = 0;

				for (int32_t i = 0; i < len; i += hexdump_line_len) {
					line++;

					int max_len = hexdump_line_len;
					if (line * hexdump_line_len > len)
						max_len = len - hexdump_line_len * (line - 1);

					printf_s("%08hhX  ", i);

					for (int j = 0; j < max_len; j++) {
						printf_s("%02hhX ", (unsigned char)buf[i + j]);
					}

					for (int j = 0; j < hexdump_line_len - max_len + 1; j++) {
						printf_s("   ");
					}

					for (int j = 0; j < max_len; j++) {
						l = (unsigned char)buf[i + j];
						printf_s("%c", (l >= 0x20) ? l : '.');
					}

					puts("");
				}

				puts("");
			}
		};

		static struct runtime_logger logger;
	}
#pragma endregion


	// String helper functions.
	// ============================================================

#pragma region String helper functions
	namespace string
	{
		inline bool starts_with(char* a, char* b)
		{
			return a != nullptr && b != nullptr && 0 == strncmp(a, b, strlen(b));
		}

		inline bool starts_with(wchar_t* a, wchar_t* b)
		{
			return a != nullptr && b != nullptr && 0 == wcsncmp(a, b, wcslen(b));
		}

		inline bool equals(char* a, char* b)
		{
			return 0 == strcmp(a, b);
		}

		inline bool equals(wchar_t* a, wchar_t* b)
		{
			return 0 == wcscmp(a, b);
		}
	}
#pragma endregion


	// SDK-reliant helper functions.
	// ============================================================

#pragma region SDK-reliant helper functions
	namespace sdk
	{
		/// <summary>
		/// A slightly reworked version of object finding routine.
		/// </summary>
		/// <typeparam name="T">UE3-SDK-originating type of the object to find.</typeparam>
		/// <returns>A vector of objects of the given type, casted to it.</returns>
		template<typename T>
		ASI_SDK_CALL std::vector<T*> find_objects()
		{
			std::vector<T*> found;

			const auto objects = UObject::GObjObjects();
			for (auto i = 0; i < objects->Count; i++)
			{
				auto object = objects->Data[i];
				if (object && object->IsA(T::StaticClass()) && !string::starts_with(object->Name.GetName(), L"Default_"))
				{
					found.push_back(reinterpret_cast<T*>(object));
				}
			}
			return found;
		}

		/// <summary>
		/// A first-hit version of sdk::find_objects.
		/// </summary>
		/// <typeparam name="T">UE3-SDK-originating type of the object to find.</typeparam>
		/// <returns>The first found object of the given type.</returns>
		template<typename T>
		ASI_SDK_CALL T* find_object()
		{
			const auto objects = UObject::GObjObjects();
			for (auto i = 0; i < objects->Count; i++)
			{
				auto object = objects->Data[i];
				if (object && object->IsA(T::StaticClass()) && !string::starts_with(object->Name.GetName(), L"Default_"))
				{
					return reinterpret_cast<T*>(object);
				}
			}
			return nullptr;
		}

		/// <summary>
		/// Get the "short name" of an Unreal object.
		/// </summary>
		/// <param name="obj">Source object.</param>
		/// <returns>Source object's name without type or hierarchy.</returns>
		ASI_SDK_CALL char* name_of(UObject* obj)
		{
			return obj ? obj->GetName() : "(null)";
		}

		/// <summary>
		/// Get the "full name" of an Unreal object.
		/// </summary>
		/// <param name="obj">Source object.</param>
		/// <returns>Source object's type and name with its (usually) complete hierarchy.</returns>
		ASI_SDK_CALL char* fullname_of(UObject* obj)
		{
			return obj ? obj->GetFullName() : "(null)";
		}

		/// <summary>
		/// Create a vector container from UE's array.
		/// </summary>
		/// <typeparam name="T">Item type.</typeparam>
		/// <param name="arr">Source UE3 array.</param>
		/// <returns>A vector with copies of the items in the source array.</returns>
		template<typename T>
		ASI_SDK_CALL std::vector<T> to_vector(TArray<T> arr)
		{
			std::vector<T> vec;

			for (int i = 0; i < arr.Count; i++)
			{
				vec.push_back(arr(i));
			}

			return vec;
		}
	}
#pragma endregion
}
