// Copyright (c) eBPF for Windows contributors
// SPDX-License-Identifier: MIT

#pragma once

#include "ebpf_core_structs.h"
#include "ebpf_execution_type.h"
#include "ebpf_program_attach_type_guids.h"
#include "ebpf_result.h"

#include <specstrings.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#include <stdexcept>
#define EBPF_NO_EXCEPT noexcept
extern "C"
{
#else
#define EBPF_NO_EXCEPT
#endif

    typedef int32_t fd_t;
    extern __declspec(selectany) const fd_t ebpf_fd_invalid = -1;
    typedef intptr_t ebpf_handle_t;

    struct bpf_object;
    struct bpf_program;
    struct bpf_map;
    struct bpf_link;

    /**
     * @brief Query info about an eBPF program.
     * @param[in] fd File descriptor of an eBPF program.
     * @param[out] execution_type On success, contains the execution type.
     * @param[out] file_name On success, contains the file name.
     * @param[out] section_name On success, contains the section name.
     * @retval EBPF_SUCCESS The operation was successful.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_program_query_info(
        fd_t fd,
        _Out_ ebpf_execution_type_t* execution_type,
        _Outptr_result_z_ const char** file_name,
        _Outptr_result_z_ const char** section_name) EBPF_NO_EXCEPT;

    typedef struct _ebpf_stat
    {
        struct _ebpf_stat* next;
        _Field_z_ const char* key;
        int value;
    } ebpf_stat_t;

    typedef struct _ebpf_api_program_info
    {
        struct _ebpf_api_program_info* next;
        _Field_z_ const char* section_name;
        _Field_z_ const char* program_name;
        ebpf_program_type_t program_type;
        ebpf_attach_type_t expected_attach_type;
        size_t raw_data_size;
        _Field_size_(raw_data_size) char* raw_data;
        ebpf_stat_t* stats;
        size_t offset_in_section; // Byte offset of program in section.
    } ebpf_api_program_info_t;

// The type ebpf_section_info_t was replaced by ebpf_api_program_info_t
// which also added the offset_in_section field at the end.
#define ebpf_section_info_t ebpf_api_program_info_t

    /**
-     * @brief Get list of programs and stats in an eBPF file.
-     * @param[in] file Name of file containing eBPF programs.
-     * @param[in] verbose Obtain additional info about the programs.
-     * @param[out] infos On success points to a list of eBPF programs.
      * The caller is responsible for freeing the list via ebpf_free_programs().
-     * @param[out] error_message On failure points to a text description of
-     *  the error.
      */
    _Must_inspect_result_ ebpf_result_t
    ebpf_enumerate_programs(
        _In_z_ const char* file,
        bool verbose,
        _Outptr_result_maybenull_ ebpf_api_program_info_t** infos,
        _Outptr_result_maybenull_z_ const char** error_message) EBPF_NO_EXCEPT;

    /**
     * @brief Get list of sections and stats in an eBPF file.
     * @param[in] file Name of file containing eBPF program sections.
     * @param[in] verbose Obtain additional info about the program sections.
     * @param[out] infos On success points to a list of eBPF program sections.
     * The caller is responsible for freeing the list via ebpf_free_sections().
     * @param[out] error_message On failure points to a text description of
     *  the error.
     * @deprecated Use ebpf_enumerate_programs() instead.
     */
    __declspec(deprecated("Use ebpf_enumerate_programs() instead.")) _Must_inspect_result_ ebpf_result_t
    ebpf_enumerate_sections(
        _In_z_ const char* file,
        bool verbose,
        _Outptr_result_maybenull_ ebpf_section_info_t** infos,
        _Outptr_result_maybenull_z_ const char** error_message) EBPF_NO_EXCEPT;

    /**
     * @brief Free memory returned from \ref ebpf_enumerate_programs.
     * @param[in] data Memory to free.
     */
    void
    ebpf_free_programs(_In_opt_ _Post_invalid_ ebpf_api_program_info_t* infos) EBPF_NO_EXCEPT;

    /**
     * @brief Free memory returned from \ref ebpf_enumerate_sections.
     * @param[in] data Memory to free.
     * @deprecated Use ebpf_free_programs() instead.
     */
    __declspec(deprecated("Use ebpf_free_programs() instead.")) void
    ebpf_free_sections(_In_opt_ _Post_invalid_ ebpf_section_info_t* infos) EBPF_NO_EXCEPT;

    /**
     * @brief Convert an eBPF program to human readable byte code.
     * @param[in] file Name of ELF file containing eBPF program.
     * @param[in] section_name The name of the section to disassemble.
     *  If NULL, the first program section is used.
     * @param[in] program_name The name of the program to disassemble.
     *  If NULL, the first program in the section is used.
     * @param[out] disassembly On success points text version of the program.
     * @param[out] error_message On failure points to a text description of
     *  the error.
     */
    uint32_t
    ebpf_api_elf_disassemble_program(
        _In_z_ const char* file,
        _In_opt_z_ const char* section_name,
        _In_opt_z_ const char* program_name,
        _Outptr_result_maybenull_z_ const char** disassembly,
        _Outptr_result_maybenull_z_ const char** error_message) EBPF_NO_EXCEPT;

    /**
     * @brief Convert an eBPF program to human readable byte code.
     * @param[in] file Name of ELF file containing eBPF program.
     * @param[in] section The name of the section to disassemble.
     * @param[out] disassembly On success points text version of the program.
     * @param[out] error_message On failure points to a text description of
     *  the error.
     */
    __declspec(deprecated("Use ebpf_api_elf_disassemble_program() instead.")) uint32_t
    ebpf_api_elf_disassemble_section(
        _In_z_ const char* file,
        _In_z_ const char* section,
        _Outptr_result_maybenull_z_ const char** disassembly,
        _Outptr_result_maybenull_z_ const char** error_message) EBPF_NO_EXCEPT;

    typedef struct
    {
        int total_unreachable;
        int total_warnings;
        int max_loop_count;
    } ebpf_api_verifier_stats_t;

    typedef enum _ebpf_verification_verbosity
    {
        EBPF_VERIFICATION_VERBOSITY_NORMAL = 0,
        EBPF_VERIFICATION_VERBOSITY_INFORMATIONAL = 1,
        EBPF_VERIFICATION_VERBOSITY_VERBOSE = 2,
    } ebpf_verification_verbosity_t;

    /**
     * @brief Verify that the program is safe to execute.
     * @param[in] file Name of ELF file containing eBPF program.
     * @param[in] section_name The name of the section in which the program exists.
     *  If NULL, the first code section is used.
     * @param[in] program_name The name of the program to verify.
     *  If NULL, the first program in the section is used.
     * @param[in] program_type Optional program type.
     *  If NULL, the program type is derived from the section name.
     * @param[in] verbosity How much additional info about the programs to obtain.
     * @param[out] report Points to a text section describing why the program
     *  failed verification.
     * @param[out] error_message On failure points to a text description of
     *  the error.
     * @param[out] stats If non-NULL, returns verification statistics.
     * @retval 0 Verification succeeded.
     * @retval 1 Verification failed.
     */
    _Success_(return == 0) uint32_t ebpf_api_elf_verify_program_from_file(
        _In_z_ const char* file,
        _In_opt_z_ const char* section_name,
        _In_opt_z_ const char* program_name,
        _In_opt_ const ebpf_program_type_t* program_type,
        ebpf_verification_verbosity_t verbosity,
        _Outptr_result_maybenull_z_ const char** report,
        _Outptr_result_maybenull_z_ const char** error_message,
        _Out_opt_ ebpf_api_verifier_stats_t* stats) EBPF_NO_EXCEPT;

    /**
     * @brief Verify that the program is safe to execute.
     * @param[in] file Name of ELF file containing eBPF program.
     * @param[in] section The name of the section to verify.
     * @param[in] program_type Optional program type.
     *  If NULL, the program type is derived from the section name.
     * @param[in] verbosity How much additional info about the programs to obtain.
     * @param[out] report Points to a text section describing why the program
     *  failed verification.
     * @param[out] error_message On failure points to a text description of
     *  the error.
     * @param[out] stats If non-NULL, returns verification statistics.
     * @retval 0 Verification succeeded.
     * @retval 1 Verification failed.
     */
    __declspec(deprecated("Use ebpf_api_elf_verify_program_from_file() instead.")) _Success_(return == 0) uint32_t
        ebpf_api_elf_verify_section_from_file(
            _In_z_ const char* file,
            _In_z_ const char* section,
            _In_opt_ const ebpf_program_type_t* program_type,
            ebpf_verification_verbosity_t verbosity,
            _Outptr_result_maybenull_z_ const char** report,
            _Outptr_result_maybenull_z_ const char** error_message,
            _Out_opt_ ebpf_api_verifier_stats_t* stats) EBPF_NO_EXCEPT;

    /**
     * @brief Verify that the program is safe to execute.
     * @param[in] data Memory containing the ELF file containing eBPF program.
     * @param[in] data_length Length of data.
     * @param[in] section_name The name of the section in which the program exists.
     *  If NULL, the first code section is used.
     * @param[in] program_name The name of the program to verify.
     *  If NULL, the first program in the section is used.
     * @param[in] program_type Optional program type.
     *  If NULL, the program type is derived from the section name.
     * @param[in] verbosity How much additional info about the programs to obtain.
     * @param[out] report Points to a text section describing why the program
     *  failed verification.
     * @param[out] error_message On failure points to a text description of
     *  the error.
     * @param[out] stats If non-NULL, returns verification statistics.
     * @retval 0 Verification succeeded.
     * @retval 1 Verification failed.
     */
    _Success_(return == 0) uint32_t ebpf_api_elf_verify_program_from_memory(
        _In_reads_(data_length) const char* data,
        size_t data_length,
        _In_opt_z_ const char* section_name,
        _In_opt_z_ const char* program_name,
        _In_opt_ const ebpf_program_type_t* program_type,
        ebpf_verification_verbosity_t verbosity,
        _Outptr_result_maybenull_z_ const char** report,
        _Outptr_result_maybenull_z_ const char** error_message,
        _Out_opt_ ebpf_api_verifier_stats_t* stats) EBPF_NO_EXCEPT;

    /**
     * @brief Verify that the program is safe to execute.
     * @param[in] data Memory containing the ELF file containing eBPF program.
     * @param[in] data_length Length of data.
     * @param[in] section_name The name of the section to verify.
     * @param[in] program_type Optional program type.
     *  If NULL, the program type is derived from the section name.
     * @param[in] verbosity How much additional info about the programs to obtain.
     * @param[out] report Points to a text section describing why the program
     *  failed verification.
     * @param[out] error_message On failure points to a text description of
     *  the error.
     * @param[out] stats If non-NULL, returns verification statistics.
     * @retval 0 Verification succeeded.
     * @retval 1 Verification failed.
     */
    __declspec(deprecated("Use ebpf_api_elf_verify_program_from_memory() instead.")) _Success_(return == 0) uint32_t
        ebpf_api_elf_verify_section_from_memory(
            _In_reads_(data_length) const char* data,
            size_t data_length,
            _In_z_ const char* section_name,
            _In_opt_ const ebpf_program_type_t* program_type,
            ebpf_verification_verbosity_t verbosity,
            _Outptr_result_maybenull_z_ const char** report,
            _Outptr_result_maybenull_z_ const char** error_message,
            _Out_opt_ ebpf_api_verifier_stats_t* stats) EBPF_NO_EXCEPT;

    /**
     * @brief Free memory for a string returned from an eBPF API.
     * @param[in] string Memory to free.
     */
    void
    ebpf_free_string(_In_opt_ _Post_invalid_ const char* string) EBPF_NO_EXCEPT;

    /**
     * @brief Dissociate a name with an object handle.
     * @param[in] name Name to dissociate.
     * @param[in] name_length Length in bytes of the name.
     */
    uint32_t
    ebpf_api_unpin_object(const uint8_t* name, uint32_t name_length) EBPF_NO_EXCEPT;

    /**
     * @brief Unpin the object from the specified path.
     * @param[in] path Path from which to unpin.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_object_unpin(_In_z_ const char* path) EBPF_NO_EXCEPT;

    /**
     * @brief Obtain information about the eBPF object referred to by bpf_fd.
     * This function populates up to info_len bytes of info, which will
     * be in one of the following formats depending on the eBPF object type of
     * bpf_fd:
     *
     * * struct bpf_link_info
     * * struct bpf_map_info
     * * struct bpf_prog_info
     *
     * @param[in] bpf_fd File descriptor referring to an eBPF object.
     * @param[in, out] info Pointer to memory in which to write the info obtained.
     * On input, contains any additional parameters to use. May be NULL in order to
     * only retrieve the type of the object.
     * @param[in, out] info_size On input, contains the maximum number of bytes to
     * write into the info. On output, contains the actual number of bytes written.
     * May be NULL if info is NULL.
     * @param[out] type Optional type of the object.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_INVALID_ARGUMENT One or more parameters are wrong.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_object_get_info_by_fd(
        fd_t bpf_fd,
        _Inout_updates_bytes_to_opt_(*info_size, *info_size) void* info,
        _Inout_opt_ uint32_t* info_size,
        _Out_opt_ ebpf_object_type_t* type) EBPF_NO_EXCEPT;

    /**
     * @brief Detach the eBPF program from the link.
     *
     * @param[in] link_handle Handle to the link.
     *
     * @retval EBPF_SUCCESS The operations succeeded.
     * @retval EBPF_INVALID_ARGUMENT The link handle is invalid.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_api_unlink_program(ebpf_handle_t link_handle) EBPF_NO_EXCEPT;

    /**
     * @brief Close an eBPF handle.
     *
     * @param[in] handle Handle to close.
     * @retval EBPF_SUCCESS Handle was closed.
     * @retval EBPF_INVALID_OBJECT Handle is not valid.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_api_close_handle(ebpf_handle_t handle) EBPF_NO_EXCEPT;

    /**
     * @brief Returns an array of \ref ebpf_map_info_t for all pinned maps.
     *
     * @param[out] map_count Number of pinned maps.
     * @param[out] map_info Array of ebpf_map_info_t for pinned maps.
     *
     * @retval EBPF_SUCCESS The API succeeded.
     * @retval EBPF_NO_MEMORY Out of memory.
     * @retval EBPF_INVALID_ARGUMENT One or more parameters are wrong.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_api_get_pinned_map_info(
        _Out_ uint16_t* map_count,
        _Outptr_result_buffer_maybenull_(*map_count) ebpf_map_info_t** map_info) EBPF_NO_EXCEPT;

    /**
     * @brief Helper Function to free array of \ref ebpf_map_info_t allocated by
     * \ref ebpf_api_get_pinned_map_info function.
     *
     * @param[in] map_count Length of array to be freed.
     * @param[in] map_info Map to be freed.
     */
    void
    ebpf_api_map_info_free(
        uint16_t map_count,
        _In_opt_count_(map_count) _Post_ptr_invalid_ const ebpf_map_info_t* map_info) EBPF_NO_EXCEPT;

    /**
     * @brief Get the execution type for an eBPF object file.
     *
     * @param[in] object The eBPF object file.
     *
     * @returns Execution type.
     */
    ebpf_execution_type_t
    ebpf_object_get_execution_type(_In_ const struct bpf_object* object) EBPF_NO_EXCEPT;

    /**
     * @brief Set the execution type for an eBPF object file.
     *
     * @param[in, out] object The eBPF object file.
     * @param[in] execution_type Execution type to set.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_INVALID_ARGUMENT One or more parameters are incorrect.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_object_set_execution_type(_Inout_ struct bpf_object* object, ebpf_execution_type_t execution_type)
        EBPF_NO_EXCEPT;

    /**
     * @brief Load a native image from a file and return map and program file
     * descriptors.
     *
     * @param[in] file_name Path to the eBPF object file.
     * @param[in, out] count_of_maps Size of map_fds.
     * @param[in] map_fds Pre-allocated array for map file descriptors.
     * @param[in, out] count_of_programs Size of program_fds.
     * @param[in] program_fds Pre-allocated array for program file descriptors.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_NO_MEMORY Either count_of_maps or count_of_programs was too small.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_object_load_native_by_fds(
        _In_z_ const char* file_name,
        _Inout_ size_t* count_of_maps,
        _Out_writes_opt_(*count_of_maps) fd_t* map_fds,
        _Inout_ size_t* count_of_programs,
        _Out_writes_opt_(*count_of_programs) fd_t* program_fds) EBPF_NO_EXCEPT;

    /**
     * @brief Attach an eBPF program.
     *
     * @param[in] program Pointer to the eBPF program.
     * @param[in] attach_type Optionally, the attach type for attaching the program.
     *  If attach type is not specified, then the earlier provided attach type
     *  or attach type derived from section prefix will be used to attach the
     *  program.
     * @param[in] attach_params_size Size of the attach parameters.
     * @param[in] attach_parameters Optionally, attach parameters. This is an
     *  opaque flat buffer containing the attach parameters which is interpreted
     *  by the extension provider.
     * @param[out] link Pointer to ebpf_link structure.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_program_attach(
        _In_ const struct bpf_program* program,
        _In_opt_ const ebpf_attach_type_t* attach_type,
        _In_reads_bytes_opt_(attach_params_size) void* attach_parameters,
        size_t attach_params_size,
        _Outptr_ struct bpf_link** link) EBPF_NO_EXCEPT;

    /**
     * @brief Attach an eBPF program by program file descriptor.
     *
     * @param[in] program_fd An eBPF program file descriptor.
     * @param[in] attach_type Optionally, the attach type for attaching the program.
     *  If attach type is not specified, then the earlier provided attach type
     *  or attach type derived from section prefix will be used to attach the
     *  program.
     * @param[in] attach_parameters_size Size of the attach parameters.
     * @param[in] attach_parameters Optionally, attach parameters. This is an
     *  opaque flat buffer containing the attach parameters which is interpreted
     *  by the extension provider.
     * @param[out] link Pointer to ebpf_link structure.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_program_attach_by_fd(
        fd_t program_fd,
        _In_opt_ const ebpf_attach_type_t* attach_type,
        _In_reads_bytes_opt_(attach_parameters_size) void* attach_parameters,
        size_t attach_parameters_size,
        _Outptr_ struct bpf_link** link) EBPF_NO_EXCEPT;

    /**
     * @brief Attach an eBPF program by program file descriptor and return
     * the link as a file descriptor.
     *
     * @see ebpf_program_attach_by_fd
     *
     * @retval EBPF_SUCCESS The operation was successful.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_program_attach_by_fds(
        fd_t program_fd,
        _In_opt_ const ebpf_attach_type_t* attach_type,
        _In_reads_bytes_opt_(attach_parameters_size) void* attach_parameters,
        size_t attach_parameters_size,
        _Out_ fd_t* link) EBPF_NO_EXCEPT;

    /**
     * @brief Detach an eBPF program from an attach point represented by
     *  the bpf_link structure.
     *
     * @param[in, out] link Pointer to bpf_link structure.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_INVALID_OBJECT Invalid object was passed.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_link_detach(_Inout_ struct bpf_link* link) EBPF_NO_EXCEPT;

    /**
     * @brief Detach an eBPF program.
     *
     * @param[in] program_fd File descriptor of program to detach. If set to -1,
     * this parameter is ignored.
     * @param[in] attach_type The attach type for attaching the program.
     * @param[in] attach_parameter_size Size of the attach parameter.
     * @param[in] attach_parameter Pointer to attach parameter. This is an
     *  opaque flat buffer containing the attach parameters which is interpreted
     *  by the extension provider.
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_INVALID_OBJECT Invalid object was passed.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_program_detach(
        fd_t program_fd,
        _In_ const ebpf_attach_type_t* attach_type,
        _In_reads_bytes_(attach_parameter_size) void* attach_parameter,
        size_t attach_parameter_size) EBPF_NO_EXCEPT;

    /**
     * Clean up and free bpf_link structure. Also close the
     * underlying link fd.
     *
     * @param[in] link Pointer to the bpf_link structure.
     *
     *
     * @sa bpf_link__destroy
     * @sa bpf_link_detach
     */
    void
    ebpf_link_close(_Frees_ptr_ struct bpf_link* link) EBPF_NO_EXCEPT;

    /**
     * @brief Close a file descriptor. Also close the underlying handle.
     * @param [in] fd File descriptor to be closed.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_INVALID_FD Invalid fd was provided.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_close_fd(fd_t fd) EBPF_NO_EXCEPT;

    /**
     * @brief Duplicate a file descriptor.
     *
     * @param [in] fd File descriptor to be duplicated.
     * @param [out] dup Duplicated file descriptor.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_duplicate_fd(fd_t fd, _Out_ fd_t* dup) EBPF_NO_EXCEPT;

    /**
     * @brief Get eBPF program type and expected attach type by name.
     *
     * @param[in] name Name, as if it were a section name in an ELF file.
     * @param[out] program_type eBPF program type.
     * @param[out] expected_attach_type Expected eBPF attach type.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_KEY_NOT_FOUND No program type was found.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_get_program_type_by_name(
        _In_z_ const char* name,
        _Out_ ebpf_program_type_t* program_type,
        _Out_ ebpf_attach_type_t* expected_attach_type) EBPF_NO_EXCEPT;

    /**
     * @brief Get the name of a given program type.
     *
     * @param[in] program_type Program type.
     *
     * @returns Name of the program type, or NULL if not found.
     */
    _Ret_maybenull_z_ const char*
    ebpf_get_program_type_name(_In_ const ebpf_program_type_t* program_type) EBPF_NO_EXCEPT;

    /**
     * @brief Get the name of a given attach type.
     *
     * @param[in] attach_type Attach type.
     *
     * @returns Name of the attach type, or NULL if not found.
     */
    _Ret_maybenull_z_ const char*
    ebpf_get_attach_type_name(_In_ const ebpf_attach_type_t* attach_type) EBPF_NO_EXCEPT;

    /**
     * @brief Gets the next pinned program after a given path.
     *
     * @param[in] start_path Path to look for an entry greater than.
     * @param[out] next_path Returns the next path, if one exists.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_NO_MORE_KEYS No more entries found.
     * @deprecated Use ebpf_get_next_pinned_object_path() instead.
     */
    __declspec(deprecated("Use ebpf_get_next_pinned_object_path() instead.")) _Must_inspect_result_ ebpf_result_t
    ebpf_get_next_pinned_program_path(
        _In_z_ const char* start_path, _Out_writes_z_(EBPF_MAX_PIN_PATH_LENGTH) char* next_path) EBPF_NO_EXCEPT;

    /**
     * @brief Retrieve the next pinned path of an eBPF object.
     *
     * @param[in] start_path Path to look for an entry greater than.
     * @param[out] next_path Returns the next path in lexicographical order, if one exists.
     * @param[in] next_path_len Length of the next path buffer.
     * @param[in, out] type On input, the type of object to retrieve or EBPF_OBJECT_UNKNOWN.
     *                      On output, the type of the object.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval other An error occurred.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_get_next_pinned_object_path(
        _In_z_ const char* start_path,
        _Out_writes_z_(next_path_len) char* next_path,
        size_t next_path_len,
        _Inout_ ebpf_object_type_t* type) EBPF_NO_EXCEPT;

    /**
     * @brief Canonicalize a path using filesystem canonicalization rules.
     *
     * @param[out] output Buffer in which to write canonicalized path.
     * @param[in] output_size Size of output buffer.
     * @param[out] error_code Zero on success, non-zero Win32 error code on failure.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_INVALID_ARGUMENT The input path was invalid.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_canonicalize_pin_path(_Out_writes_(output_size) char* output, size_t output_size, _In_z_ const char* input)
        EBPF_NO_EXCEPT;

    typedef struct _ebpf_program_info ebpf_program_info_t;

    /**
     * @brief Get the set of program information used by the verifier during
     * the last verification.
     *
     * @param[out] program_info Pointer to the program information used to
     * verify the program.
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_OBJECT_NOT_FOUND No program information was found.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_get_program_info_from_verifier(_Outptr_ const ebpf_program_info_t** program_info) EBPF_NO_EXCEPT;

    typedef struct _ebpf_test_run_options
    {
        _Readable_bytes_(data_size_in) const uint8_t* data_in; ///< Input data to the program.
        _Writable_bytes_(data_size_out) uint8_t* data_out;     ///< Output data from the program.
        size_t data_size_in;                                   ///< Size of input data.
        size_t data_size_out; ///< Maximum length of data_out on input and actual length of data_out on output.
        _Readable_bytes_(context_size_in) const uint8_t* context_in; ///< Input context to the program.
        _Writable_bytes_(context_size_out) uint8_t* context_out;     ///< Output context from the program.
        size_t context_size_in;                                      ///< Size of input context.
        size_t context_size_out; ///< Maximum length of context_out on input and actual length of context_out on output.
        uint64_t return_value;   ///< Return value from the program.
        size_t repeat_count;     ///< Number of times to repeat the program.
        uint64_t duration;       ///< Duration in nanoseconds of the program execution.
        uint32_t flags;          ///< Flags to control the test run.
        uint32_t cpu;            ///< CPU to run the program on.
        size_t batch_size;       ///< Number of times to repeat the program in a batch.
    } ebpf_test_run_options_t;

    /**
     * @brief Run the program in the eBPF VM, measure the execution time, and return the result.
     *
     * @param[in] program_fd File descriptor of the program to run.
     * @param[in,out] options Options to control the test run and results.
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_INVALID_OBJECT Invalid object was passed.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_program_test_run(fd_t program_fd, _Inout_ ebpf_test_run_options_t* options) EBPF_NO_EXCEPT;

    /**
     * @brief Write data into the ring buffer map.
     *
     * @param [in] ring_buffer_map_fd ring buffer map file descriptor.
     * @param [in]  data Pointer to data to be written.
     * @param [in] data_length Length of data to be written.
     * @retval EPBF_SUCCESS Successfully wrote record into ring buffer.
     * @retval EBPF_OUT_OF_SPACE Unable to output to ring buffer due to inadequate space.
     * @retval EBPF_NO_MEMORY Out of memory.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_ring_buffer_map_write(
        fd_t ring_buffer_map_fd, _In_reads_bytes_(data_length) const void* data, size_t data_length) EBPF_NO_EXCEPT;

    /**
     * @brief Get eBPF program type for the specified BPF program type.
     *
     * @param[in] program_type BPF program type.
     *
     * @returns Pointer to eBPF program type, or NULL if not found.
     */
    _Ret_maybenull_ const ebpf_program_type_t*
    ebpf_get_ebpf_program_type(bpf_prog_type_t bpf_program_type) EBPF_NO_EXCEPT;

    /**
     * @brief Get eBPF attach type for the specified BPF attach type.
     *
     * @param[in] bpf_attach_type BPF attach type.
     * @param[out] ebpf_attach_type eBPF attach type or GUID_NULL.
     *
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_INVALID_ARGUMENT The attach type is unknown.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_get_ebpf_attach_type(bpf_attach_type_t bpf_attach_type, _Out_ ebpf_attach_type_t* ebpf_attach_type)
        EBPF_NO_EXCEPT;

    /**
     * @brief Get BPF program type for the specified eBPF program type.
     *
     * @param[in] program_type eBPF program type GUID.
     *
     * @returns BPF program type, or BPF_PROG_TYPE_UNSPEC if not found.
     */
    bpf_prog_type_t
    ebpf_get_bpf_program_type(_In_ const ebpf_program_type_t* program_type) EBPF_NO_EXCEPT;

    /**
     * @brief Get BPF attach type for the specified eBPF attach type.
     *
     * @param[in] attach_type eBPF attach type GUID.
     *
     * @returns BPF attach type, or BPF_ATTACH_TYPE_UNSPEC if not found.
     */
    bpf_attach_type_t
    ebpf_get_bpf_attach_type(_In_ const ebpf_attach_type_t* ebpf_attach_type) EBPF_NO_EXCEPT;

    /**
     * @brief Write data into the perf event array map.
     *
     * @param [in] perf_event_array_map_fd perf event array map file descriptor.
     * @param [in] data Pointer to data to be written.
     * @param [in] data_length Length of data to be written.
     * @retval EPBF_SUCCESS Successfully wrote record into perf event array.
     * @retval EBPF_OUT_OF_SPACE Unable to output to perf event array due to inadequate space.
     * @retval EBPF_NO_MEMORY Out of memory.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_perf_event_array_map_write(
        fd_t perf_event_array_map_fd,
        _In_reads_bytes_(data_length) const void* data,
        size_t data_length) EBPF_NO_EXCEPT;

    /**
     * @brief Authorize a native module to be loaded.
     *
     * @param[in] module_path Path to the native module to be authorized.
     * @retval EBPF_SUCCESS The operation was successful.
     * @retval EBPF_NO_MEMORY Out of memory.
     */
    _Must_inspect_result_ ebpf_result_t
    ebpf_authorize_native_module(_In_z_ const char* module_path) EBPF_NO_EXCEPT;

#ifdef __cplusplus
}
#endif
