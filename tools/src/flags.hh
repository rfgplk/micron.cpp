#pragma once

#include "types.hpp"

namespace gcc
{
namespace c_flags
{
// C Language Standards and Extensions
constexpr static const i32 flag_ansi = 0;
constexpr static const i32 flag_std = 1;
constexpr static const i32 flag_aux_info = 2;
constexpr static const i32 flag_no_asm = 3;
constexpr static const i32 flag_no_builtin = 4;
constexpr static const i32 flag_no_builtin_function = 5;
constexpr static const i32 flag_cond_mismatch = 6;
constexpr static const i32 flag_freestanding = 7;
constexpr static const i32 flag_gimple = 8;
constexpr static const i32 flag_gnu_tm = 9;
constexpr static const i32 flag_gnu89_inline = 10;
constexpr static const i32 flag_hosted = 11;
constexpr static const i32 flag_lax_vector_conversions = 12;
constexpr static const i32 flag_ms_extensions = 13;
constexpr static const i32 flag_permitted_flt_eval_methods = 14;
constexpr static const i32 flag_plan9_extensions = 15;
constexpr static const i32 flag_signed_bitfields = 16;
constexpr static const i32 flag_unsigned_bitfields = 17;
constexpr static const i32 flag_signed_char = 18;
constexpr static const i32 flag_unsigned_char = 19;
constexpr static const i32 flag_strict_flex_arrays = 20;
constexpr static const i32 flag_sso_struct = 21;

enum class flags : i32 {
  ansi = flag_ansi,
  std = flag_std,
  aux_info = flag_aux_info,
  no_asm = flag_no_asm,
  no_builtin = flag_no_builtin,
  no_builtin_function = flag_no_builtin_function,
  cond_mismatch = flag_cond_mismatch,
  freestanding = flag_freestanding,
  gimple = flag_gimple,
  gnu_tm = flag_gnu_tm,
  gnu89_inline = flag_gnu89_inline,
  hosted = flag_hosted,
  lax_vector_conversions = flag_lax_vector_conversions,
  ms_extensions = flag_ms_extensions,
  permitted_flt_eval_methods = flag_permitted_flt_eval_methods,
  plan9_extensions = flag_plan9_extensions,
  signed_bitfields = flag_signed_bitfields,
  unsigned_bitfields = flag_unsigned_bitfields,
  signed_char = flag_signed_char,
  unsigned_char = flag_unsigned_char,
  strict_flex_arrays = flag_strict_flex_arrays,
  sso_struct = flag_sso_struct
};

// String literals for flags
constexpr static const char *flag_strings[] = { "-ansi",
                                                "-std=",
                                                "-aux-info",
                                                "-fno-asm",
                                                "-fno-builtin",
                                                "-fno-builtin-function",
                                                "-fcond-mismatch",
                                                "-ffreestanding",
                                                "-fgimple",
                                                "-fgnu-tm",
                                                "-fgnu89-inline",
                                                "-fhosted",
                                                "-flax-vector-conversions",
                                                "-fms-extensions",
                                                "-fpermitted-flt-eval-methods=",
                                                "-fplan9-extensions",
                                                "-fsigned-bitfields",
                                                "-funsigned-bitfields",
                                                "-fsigned-char",
                                                "-funsigned-char",
                                                "-fstrict-flex-arrays",
                                                "-fsso-struct=" };

constexpr const char *
get_string_flag(flags f)
{
  return flag_strings[static_cast<i32>(f)];
}
}

namespace cpp_flags
{
// C++ Language Features and Standards
constexpr static const i32 flag_abi_version = 0;
constexpr static const i32 flag_no_access_control = 1;
constexpr static const i32 flag_aligned_new = 2;
constexpr static const i32 flag_no_assume_sane_operators_new_delete = 3;
constexpr static const i32 flag_char8_t = 4;
constexpr static const i32 flag_check_new = 5;
constexpr static const i32 flag_concepts = 6;
constexpr static const i32 flag_constexpr_depth = 7;
constexpr static const i32 flag_constexpr_cache_depth = 8;
constexpr static const i32 flag_constexpr_loop_limit = 9;
constexpr static const i32 flag_constexpr_ops_limit = 10;
constexpr static const i32 flag_no_elide_constructors = 11;
constexpr static const i32 flag_no_enforce_eh_specs = 12;
constexpr static const i32 flag_no_gnu_keywords = 13;
constexpr static const i32 flag_no_immediate_escalation = 14;
constexpr static const i32 flag_no_implicit_templates = 15;
constexpr static const i32 flag_no_implicit_inline_templates = 16;
constexpr static const i32 flag_no_implement_inlines = 17;
constexpr static const i32 flag_module_header = 18;
constexpr static const i32 flag_module_only = 19;
constexpr static const i32 flag_modules = 20;
constexpr static const i32 flag_module_implicit_inline = 21;
constexpr static const i32 flag_no_module_lazy = 22;
constexpr static const i32 flag_module_mapper = 23;
constexpr static const i32 flag_module_version_ignore = 24;
constexpr static const i32 flag_ms_extensions_cpp = 25;
constexpr static const i32 flag_new_inheriting_ctors = 26;
constexpr static const i32 flag_new_ttp_matching = 27;
constexpr static const i32 flag_no_nonansi_builtins = 28;
constexpr static const i32 flag_nothrow_opt = 29;
constexpr static const i32 flag_no_operator_names = 30;
constexpr static const i32 flag_no_optional_diags = 31;
constexpr static const i32 flag_no_pretty_templates = 32;
constexpr static const i32 flag_range_for_ext_temps = 33;
constexpr static const i32 flag_no_rtti = 34;
constexpr static const i32 flag_sized_deallocation = 35;
constexpr static const i32 flag_strong_eval_order = 36;
constexpr static const i32 flag_template_backtrace_limit = 37;
constexpr static const i32 flag_template_depth = 38;
constexpr static const i32 flag_no_threadsafe_statics = 39;
constexpr static const i32 flag_use_cxa_atexit = 40;
constexpr static const i32 flag_no_weak = 41;
constexpr static const i32 flag_nostdinc_pp = 42;
constexpr static const i32 flag_visibility_inlines_hidden = 43;
constexpr static const i32 flag_visibility_ms_compat = 44;
constexpr static const i32 flag_ext_numeric_literals = 45;
constexpr static const i32 flag_lang_info_include_translate = 46;
constexpr static const i32 flag_lang_info_include_translate_not = 47;
constexpr static const i32 flag_lang_info_module_cmi = 48;
constexpr static const i32 flag_stdlib = 49;

// C++ Warning Flags
constexpr static const i32 flag_w_abi_tag = 50;
constexpr static const i32 flag_w_catch_value = 51;
constexpr static const i32 flag_w_no_class_conversion = 52;
constexpr static const i32 flag_w_class_memaccess = 53;
constexpr static const i32 flag_w_comma_subscript = 54;
constexpr static const i32 flag_w_conditionally_supported = 55;
constexpr static const i32 flag_w_no_conversion_null = 56;
constexpr static const i32 flag_w_ctad_maybe_unsupported = 57;
constexpr static const i32 flag_w_ctor_dtor_privacy = 58;
constexpr static const i32 flag_w_dangling_reference = 59;
constexpr static const i32 flag_w_no_defaulted_function_deleted = 60;
constexpr static const i32 flag_w_no_delete_incomplete = 61;
constexpr static const i32 flag_w_delete_non_virtual_dtor = 62;
constexpr static const i32 flag_w_no_deprecated_array_compare = 63;
constexpr static const i32 flag_w_deprecated_copy = 64;
constexpr static const i32 flag_w_deprecated_copy_dtor = 65;
constexpr static const i32 flag_w_no_deprecated_enum_enum_conversion = 66;
constexpr static const i32 flag_w_no_deprecated_enum_float_conversion = 67;
constexpr static const i32 flag_w_effc_pp = 68;
constexpr static const i32 flag_w_no_elaborated_enum_base = 69;
constexpr static const i32 flag_w_no_exceptions = 70;
constexpr static const i32 flag_w_extra_semi = 71;
constexpr static const i32 flag_w_no_global_module = 72;
constexpr static const i32 flag_w_no_inaccessible_base = 73;
constexpr static const i32 flag_w_no_inherited_variadic_ctor = 74;
constexpr static const i32 flag_w_no_init_list_lifetime = 75;
constexpr static const i32 flag_w_invalid_constexpr = 76;
constexpr static const i32 flag_w_invalid_imported_macros = 77;
constexpr static const i32 flag_w_no_invalid_offsetof = 78;
constexpr static const i32 flag_w_no_literal_suffix = 79;
constexpr static const i32 flag_w_mismatched_new_delete = 80;
constexpr static const i32 flag_w_mismatched_tags = 81;
constexpr static const i32 flag_w_multiple_inheritance = 82;
constexpr static const i32 flag_w_namespaces = 83;
constexpr static const i32 flag_w_narrowing = 84;
constexpr static const i32 flag_w_noexcept = 85;
constexpr static const i32 flag_w_noexcept_type = 86;
constexpr static const i32 flag_w_non_virtual_dtor = 87;
constexpr static const i32 flag_w_pessimizing_move = 88;
constexpr static const i32 flag_w_no_placement_new = 89;
constexpr static const i32 flag_w_placement_new = 90;
constexpr static const i32 flag_w_range_loop_construct = 91;
constexpr static const i32 flag_w_redundant_move = 92;
constexpr static const i32 flag_w_redundant_tags = 93;
constexpr static const i32 flag_w_reorder = 94;
constexpr static const i32 flag_w_register = 95;
constexpr static const i32 flag_w_strict_null_sentinel = 96;
constexpr static const i32 flag_w_no_subobject_linkage = 97;
constexpr static const i32 flag_w_templates = 98;
constexpr static const i32 flag_w_no_non_template_friend = 99;
constexpr static const i32 flag_w_old_style_cast = 100;
constexpr static const i32 flag_w_overloaded_virtual = 101;
constexpr static const i32 flag_w_no_pmf_conversions = 102;
constexpr static const i32 flag_w_self_move = 103;
constexpr static const i32 flag_w_sign_promo = 104;
constexpr static const i32 flag_w_sized_deallocation = 105;
constexpr static const i32 flag_w_suggest_final_methods = 106;
constexpr static const i32 flag_w_suggest_final_types = 107;
constexpr static const i32 flag_w_suggest_override = 108;
constexpr static const i32 flag_w_no_template_body = 109;
constexpr static const i32 flag_w_no_template_id_cdtor = 110;
constexpr static const i32 flag_w_template_names_tu_local = 111;
constexpr static const i32 flag_w_no_terminate = 112;
constexpr static const i32 flag_w_no_vexing_parse = 113;
constexpr static const i32 flag_w_virtual_inheritance = 114;
constexpr static const i32 flag_w_no_virtual_move_assign = 115;
constexpr static const i32 flag_w_volatile = 116;

enum class flags : i32 {
  abi_version = flag_abi_version,
  no_access_control = flag_no_access_control,
  aligned_new = flag_aligned_new,
  no_assume_sane_operators_new_delete = flag_no_assume_sane_operators_new_delete,
  fchar8_t = flag_char8_t,
  check_new = flag_check_new,
  concepts = flag_concepts,
  constexpr_depth = flag_constexpr_depth,
  constexpr_cache_depth = flag_constexpr_cache_depth,
  constexpr_loop_limit = flag_constexpr_loop_limit,
  constexpr_ops_limit = flag_constexpr_ops_limit,
  no_elide_constructors = flag_no_elide_constructors,
  no_enforce_eh_specs = flag_no_enforce_eh_specs,
  no_gnu_keywords = flag_no_gnu_keywords,
  no_immediate_escalation = flag_no_immediate_escalation,
  no_implicit_templates = flag_no_implicit_templates,
  no_implicit_inline_templates = flag_no_implicit_inline_templates,
  no_implement_inlines = flag_no_implement_inlines,
  module_header = flag_module_header,
  module_only = flag_module_only,
  modules = flag_modules,
  module_implicit_inline = flag_module_implicit_inline,
  no_module_lazy = flag_no_module_lazy,
  module_mapper = flag_module_mapper,
  module_version_ignore = flag_module_version_ignore,
  ms_extensions_cpp = flag_ms_extensions_cpp,
  new_inheriting_ctors = flag_new_inheriting_ctors,
  new_ttp_matching = flag_new_ttp_matching,
  no_nonansi_builtins = flag_no_nonansi_builtins,
  nothrow_opt = flag_nothrow_opt,
  no_operator_names = flag_no_operator_names,
  no_optional_diags = flag_no_optional_diags,
  no_pretty_templates = flag_no_pretty_templates,
  range_for_ext_temps = flag_range_for_ext_temps,
  no_rtti = flag_no_rtti,
  sized_deallocation = flag_sized_deallocation,
  strong_eval_order = flag_strong_eval_order,
  template_backtrace_limit = flag_template_backtrace_limit,
  template_depth = flag_template_depth,
  no_threadsafe_statics = flag_no_threadsafe_statics,
  use_cxa_atexit = flag_use_cxa_atexit,
  no_weak = flag_no_weak,
  nostdinc_pp = flag_nostdinc_pp,
  visibility_inlines_hidden = flag_visibility_inlines_hidden,
  visibility_ms_compat = flag_visibility_ms_compat,
  ext_numeric_literals = flag_ext_numeric_literals,
  lang_info_include_translate = flag_lang_info_include_translate,
  lang_info_include_translate_not = flag_lang_info_include_translate_not,
  lang_info_module_cmi = flag_lang_info_module_cmi,
  stdlib = flag_stdlib,
  w_abi_tag = flag_w_abi_tag,
  w_catch_value = flag_w_catch_value,
  w_no_class_conversion = flag_w_no_class_conversion,
  w_class_memaccess = flag_w_class_memaccess,
  w_comma_subscript = flag_w_comma_subscript,
  w_conditionally_supported = flag_w_conditionally_supported,
  w_no_conversion_null = flag_w_no_conversion_null,
  w_ctad_maybe_unsupported = flag_w_ctad_maybe_unsupported,
  w_ctor_dtor_privacy = flag_w_ctor_dtor_privacy,
  w_dangling_reference = flag_w_dangling_reference,
  w_no_defaulted_function_deleted = flag_w_no_defaulted_function_deleted,
  w_no_delete_incomplete = flag_w_no_delete_incomplete,
  w_delete_non_virtual_dtor = flag_w_delete_non_virtual_dtor,
  w_no_deprecated_array_compare = flag_w_no_deprecated_array_compare,
  w_deprecated_copy = flag_w_deprecated_copy,
  w_deprecated_copy_dtor = flag_w_deprecated_copy_dtor,
  w_no_deprecated_enum_enum_conversion = flag_w_no_deprecated_enum_enum_conversion,
  w_no_deprecated_enum_float_conversion = flag_w_no_deprecated_enum_float_conversion,
  w_effc_pp = flag_w_effc_pp,
  w_no_elaborated_enum_base = flag_w_no_elaborated_enum_base,
  w_no_exceptions = flag_w_no_exceptions,
  w_extra_semi = flag_w_extra_semi,
  w_no_global_module = flag_w_no_global_module,
  w_no_inaccessible_base = flag_w_no_inaccessible_base,
  w_no_inherited_variadic_ctor = flag_w_no_inherited_variadic_ctor,
  w_no_init_list_lifetime = flag_w_no_init_list_lifetime,
  w_invalid_constexpr = flag_w_invalid_constexpr,
  w_invalid_imported_macros = flag_w_invalid_imported_macros,
  w_no_invalid_offsetof = flag_w_no_invalid_offsetof,
  w_no_literal_suffix = flag_w_no_literal_suffix,
  w_mismatched_new_delete = flag_w_mismatched_new_delete,
  w_mismatched_tags = flag_w_mismatched_tags,
  w_multiple_inheritance = flag_w_multiple_inheritance,
  w_namespaces = flag_w_namespaces,
  w_narrowing = flag_w_narrowing,
  w_noexcept = flag_w_noexcept,
  w_noexcept_type = flag_w_noexcept_type,
  w_non_virtual_dtor = flag_w_non_virtual_dtor,
  w_pessimizing_move = flag_w_pessimizing_move,
  w_no_placement_new = flag_w_no_placement_new,
  w_placement_new = flag_w_placement_new,
  w_range_loop_construct = flag_w_range_loop_construct,
  w_redundant_move = flag_w_redundant_move,
  w_redundant_tags = flag_w_redundant_tags,
  w_reorder = flag_w_reorder,
  w_register = flag_w_register,
  w_strict_null_sentinel = flag_w_strict_null_sentinel,
  w_no_subobject_linkage = flag_w_no_subobject_linkage,
  w_templates = flag_w_templates,
  w_no_non_template_friend = flag_w_no_non_template_friend,
  w_old_style_cast = flag_w_old_style_cast,
  w_overloaded_virtual = flag_w_overloaded_virtual,
  w_no_pmf_conversions = flag_w_no_pmf_conversions,
  w_self_move = flag_w_self_move,
  w_sign_promo = flag_w_sign_promo,
  w_sized_deallocation = flag_w_sized_deallocation,
  w_suggest_final_methods = flag_w_suggest_final_methods,
  w_suggest_final_types = flag_w_suggest_final_types,
  w_suggest_override = flag_w_suggest_override,
  w_no_template_body = flag_w_no_template_body,
  w_no_template_id_cdtor = flag_w_no_template_id_cdtor,
  w_template_names_tu_local = flag_w_template_names_tu_local,
  w_no_terminate = flag_w_no_terminate,
  w_no_vexing_parse = flag_w_no_vexing_parse,
  w_virtual_inheritance = flag_w_virtual_inheritance,
  w_no_virtual_move_assign = flag_w_no_virtual_move_assign,
  w_volatile = flag_w_volatile
};

// String literals for C++ flags
constexpr static const char *flag_strings[] = { "-fabi-version=",
                                                "-fno-access-control",
                                                "-faligned-new=",
                                                "-fno-assume-sane-operators-new-delete",
                                                "-fchar8_t",
                                                "-fcheck-new",
                                                "-fconcepts",
                                                "-fconstexpr-depth=",
                                                "-fconstexpr-cache-depth=",
                                                "-fconstexpr-loop-limit=",
                                                "-fconstexpr-ops-limit=",
                                                "-fno-elide-constructors",
                                                "-fno-enforce-eh-specs",
                                                "-fno-gnu-keywords",
                                                "-fno-immediate-escalation",
                                                "-fno-implicit-templates",
                                                "-fno-implicit-inline-templates",
                                                "-fno-implement-inlines",
                                                "-fmodule-header",
                                                "-fmodule-only",
                                                "-fmodules",
                                                "-fmodule-implicit-inline",
                                                "-fno-module-lazy",
                                                "-fmodule-mapper=",
                                                "-fmodule-version-ignore",
                                                "-fms-extensions",
                                                "-fnew-inheriting-ctors",
                                                "-fnew-ttp-matching",
                                                "-fno-nonansi-builtins",
                                                "-fnothrow-opt",
                                                "-fno-operator-names",
                                                "-fno-optional-diags",
                                                "-fno-pretty-templates",
                                                "-frange-for-ext-temps",
                                                "-fno-rtti",
                                                "-fsized-deallocation",
                                                "-fstrong-eval-order",
                                                "-ftemplate-backtrace-limit=",
                                                "-ftemplate-depth=",
                                                "-fno-threadsafe-statics",
                                                "-fuse-cxa-atexit",
                                                "-fno-weak",
                                                "-nostdinc++",
                                                "-fvisibility-inlines-hidden",
                                                "-fvisibility-ms-compat",
                                                "-fext-numeric-literals",
                                                "-flang-info-include-translate",
                                                "-flang-info-include-translate-not",
                                                "-flang-info-module-cmi",
                                                "-stdlib=",
                                                "-Wabi-tag",
                                                "-Wcatch-value",
                                                "-Wno-class-conversion",
                                                "-Wclass-memaccess",
                                                "-Wcomma-subscript",
                                                "-Wconditionally-supported",
                                                "-Wno-conversion-null",
                                                "-Wctad-maybe-unsupported",
                                                "-Wctor-dtor-privacy",
                                                "-Wdangling-reference",
                                                "-Wno-defaulted-function-deleted",
                                                "-Wno-delete-incomplete",
                                                "-Wdelete-non-virtual-dtor",
                                                "-Wno-deprecated-array-compare",
                                                "-Wdeprecated-copy",
                                                "-Wdeprecated-copy-dtor",
                                                "-Wno-deprecated-enum-enum-conversion",
                                                "-Wno-deprecated-enum-float-conversion",
                                                "-Weffc++",
                                                "-Wno-elaborated-enum-base",
                                                "-Wno-exceptions",
                                                "-Wextra-semi",
                                                "-Wno-global-module",
                                                "-Wno-inaccessible-base",
                                                "-Wno-inherited-variadic-ctor",
                                                "-Wno-init-list-lifetime",
                                                "-Winvalid-constexpr",
                                                "-Winvalid-imported-macros",
                                                "-Wno-invalid-offsetof",
                                                "-Wno-literal-suffix",
                                                "-Wmismatched-new-delete",
                                                "-Wmismatched-tags",
                                                "-Wmultiple-inheritance",
                                                "-Wnamespaces",
                                                "-Wnarrowing",
                                                "-Wnoexcept",
                                                "-Wnoexcept-type",
                                                "-Wnon-virtual-dtor",
                                                "-Wpessimizing-move",
                                                "-Wno-placement-new",
                                                "-Wplacement-new=",
                                                "-Wrange-loop-construct",
                                                "-Wredundant-move",
                                                "-Wredundant-tags",
                                                "-Wreorder",
                                                "-Wregister",
                                                "-Wstrict-null-sentinel",
                                                "-Wno-subobject-linkage",
                                                "-Wtemplates",
                                                "-Wno-non-template-friend",
                                                "-Wold-style-cast",
                                                "-Woverloaded-virtual",
                                                "-Wno-pmf-conversions",
                                                "-Wself-move",
                                                "-Wsign-promo",
                                                "-Wsized-deallocation",
                                                "-Wsuggest-final-methods",
                                                "-Wsuggest-final-types",
                                                "-Wsuggest-override",
                                                "-Wno-template-body",
                                                "-Wno-template-id-cdtor",
                                                "-Wtemplate-names-tu-local",
                                                "-Wno-terminate",
                                                "-Wno-vexing-parse",
                                                "-Wvirtual-inheritance",
                                                "-Wno-virtual-move-assign",
                                                "-Wvolatile" };

constexpr const char *
get_string_flag(flags f)
{
  return flag_strings[static_cast<i32>(f)];
}
}

namespace diagnostic_flags
{
constexpr static const i32 flag_message_length = 0;
constexpr static const i32 flag_diagnostics_plain_output = 1;
constexpr static const i32 flag_diagnostics_show_location = 2;
constexpr static const i32 flag_diagnostics_color = 3;
constexpr static const i32 flag_diagnostics_urls = 4;
constexpr static const i32 flag_diagnostics_format = 5;
constexpr static const i32 flag_diagnostics_add_output = 6;
constexpr static const i32 flag_diagnostics_set_output = 7;
constexpr static const i32 flag_no_diagnostics_json_formatting = 8;
constexpr static const i32 flag_no_diagnostics_show_option = 9;
constexpr static const i32 flag_no_diagnostics_show_caret = 10;
constexpr static const i32 flag_no_diagnostics_show_event_links = 11;
constexpr static const i32 flag_no_diagnostics_show_labels = 12;
constexpr static const i32 flag_no_diagnostics_show_line_numbers = 13;
constexpr static const i32 flag_no_diagnostics_show_cwe = 14;
constexpr static const i32 flag_no_diagnostics_show_rules = 15;
constexpr static const i32 flag_no_diagnostics_show_highlight_colors = 16;
constexpr static const i32 flag_diagnostics_minimum_margin_width = 17;
constexpr static const i32 flag_diagnostics_parseable_fixits = 18;
constexpr static const i32 flag_diagnostics_generate_patch = 19;
constexpr static const i32 flag_diagnostics_show_template_tree = 20;
constexpr static const i32 flag_no_elide_type = 21;
constexpr static const i32 flag_diagnostics_path_format = 22;
constexpr static const i32 flag_diagnostics_show_path_depths = 23;
constexpr static const i32 flag_no_show_column = 24;
constexpr static const i32 flag_diagnostics_column_unit = 25;
constexpr static const i32 flag_diagnostics_column_origin = 26;
constexpr static const i32 flag_diagnostics_escape_format = 27;
constexpr static const i32 flag_diagnostics_text_art_charset = 28;

enum class flags : i32 {
  message_length = flag_message_length,
  diagnostics_plain_output = flag_diagnostics_plain_output,
  diagnostics_show_location = flag_diagnostics_show_location,
  diagnostics_color = flag_diagnostics_color,
  diagnostics_urls = flag_diagnostics_urls,
  diagnostics_format = flag_diagnostics_format,
  diagnostics_add_output = flag_diagnostics_add_output,
  diagnostics_set_output = flag_diagnostics_set_output,
  no_diagnostics_json_formatting = flag_no_diagnostics_json_formatting,
  no_diagnostics_show_option = flag_no_diagnostics_show_option,
  no_diagnostics_show_caret = flag_no_diagnostics_show_caret,
  no_diagnostics_show_event_links = flag_no_diagnostics_show_event_links,
  no_diagnostics_show_labels = flag_no_diagnostics_show_labels,
  no_diagnostics_show_line_numbers = flag_no_diagnostics_show_line_numbers,
  no_diagnostics_show_cwe = flag_no_diagnostics_show_cwe,
  no_diagnostics_show_rules = flag_no_diagnostics_show_rules,
  no_diagnostics_show_highlight_colors = flag_no_diagnostics_show_highlight_colors,
  diagnostics_minimum_margin_width = flag_diagnostics_minimum_margin_width,
  diagnostics_parseable_fixits = flag_diagnostics_parseable_fixits,
  diagnostics_generate_patch = flag_diagnostics_generate_patch,
  diagnostics_show_template_tree = flag_diagnostics_show_template_tree,
  no_elide_type = flag_no_elide_type,
  diagnostics_path_format = flag_diagnostics_path_format,
  diagnostics_show_path_depths = flag_diagnostics_show_path_depths,
  no_show_column = flag_no_show_column,
  diagnostics_column_unit = flag_diagnostics_column_unit,
  diagnostics_column_origin = flag_diagnostics_column_origin,
  diagnostics_escape_format = flag_diagnostics_escape_format,
  diagnostics_text_art_charset = flag_diagnostics_text_art_charset
};
}

namespace w_flags
{

// Warning flag indices
constexpr static const i32 flag_fsyntax_only = 0;
constexpr static const i32 flag_fmax_errors = 1;
constexpr static const i32 flag_pedantic = 2;
constexpr static const i32 flag_pedantic_errors = 3;
constexpr static const i32 flag_fpermissive = 4;
constexpr static const i32 flag_w = 5;
constexpr static const i32 flag_w_extra = 6;
constexpr static const i32 flag_w_all = 7;
constexpr static const i32 flag_w_abi = 8;
constexpr static const i32 flag_w_address = 9;
constexpr static const i32 flag_w_aggregate_return = 10;
constexpr static const i32 flag_w_alloc_size = 11;
constexpr static const i32 flag_w_alloc_zero = 12;
constexpr static const i32 flag_w_alloca = 13;
constexpr static const i32 flag_w_arith_conversion = 14;
constexpr static const i32 flag_w_array_bounds = 15;
constexpr static const i32 flag_w_array_compare = 16;
constexpr static const i32 flag_w_array_parameter = 17;
constexpr static const i32 flag_w_attribute_alias = 18;
constexpr static const i32 flag_w_bool_compare = 19;
constexpr static const i32 flag_w_bool_operation = 20;
constexpr static const i32 flag_w_c90_c99_compat = 21;
constexpr static const i32 flag_w_c99_c11_compat = 22;
constexpr static const i32 flag_w_c11_c23_compat = 23;
constexpr static const i32 flag_w_c23_c2y_compat = 24;
constexpr static const i32 flag_w_cxx_compat = 25;
constexpr static const i32 flag_w_cast_align = 26;
constexpr static const i32 flag_w_cast_function_type = 27;
constexpr static const i32 flag_w_cast_qual = 28;
constexpr static const i32 flag_w_char_subscripts = 29;
constexpr static const i32 flag_w_clobbered = 30;
constexpr static const i32 flag_w_comment = 31;
constexpr static const i32 flag_w_conversion = 32;
constexpr static const i32 flag_w_dangling_pointer = 33;
constexpr static const i32 flag_w_date_time = 34;
constexpr static const i32 flag_w_double_promotion = 35;
constexpr static const i32 flag_w_duplicated_branches = 36;
constexpr static const i32 flag_w_duplicated_cond = 37;
constexpr static const i32 flag_w_empty_body = 38;
constexpr static const i32 flag_w_enum_compare = 39;
constexpr static const i32 flag_w_enum_conversion = 40;
constexpr static const i32 flag_w_error = 41;
constexpr static const i32 flag_w_fatal_errors = 42;
constexpr static const i32 flag_w_float_conversion = 43;
constexpr static const i32 flag_w_float_equal = 44;
constexpr static const i32 flag_w_format = 45;
constexpr static const i32 flag_w_format_security = 46;
constexpr static const i32 flag_w_frame_address = 47;
constexpr static const i32 flag_w_header_guard = 48;
constexpr static const i32 flag_w_implicit_fallthrough = 49;
constexpr static const i32 flag_w_infinite_recursion = 50;
constexpr static const i32 flag_w_init_self = 51;
constexpr static const i32 flag_w_inline = 52;
constexpr static const i32 flag_w_int_in_bool_context = 53;
constexpr static const i32 flag_w_invalid_utf8 = 54;
constexpr static const i32 flag_w_jump_misses_init = 55;
constexpr static const i32 flag_w_logical_op = 56;
constexpr static const i32 flag_w_long_long = 57;
constexpr static const i32 flag_w_maybe_uninitialized = 58;
constexpr static const i32 flag_w_misleading_indentation = 59;
constexpr static const i32 flag_w_missing_braces = 60;
constexpr static const i32 flag_w_missing_field_initializers = 61;
constexpr static const i32 flag_w_nonnull = 62;
constexpr static const i32 flag_w_null_dereference = 63;
constexpr static const i32 flag_w_overlength_strings = 64;
constexpr static const i32 flag_w_packed = 65;
constexpr static const i32 flag_w_padded = 66;
constexpr static const i32 flag_w_parentheses = 67;
constexpr static const i32 flag_w_pointer_arith = 68;
constexpr static const i32 flag_w_redundant_decls = 69;
constexpr static const i32 flag_w_restrict = 70;
constexpr static const i32 flag_w_return_type = 71;
constexpr static const i32 flag_w_sequence_point = 72;
constexpr static const i32 flag_w_shadow = 73;
constexpr static const i32 flag_w_sign_compare = 74;
constexpr static const i32 flag_w_sign_conversion = 75;
constexpr static const i32 flag_w_sizeof_pointer_memaccess = 76;
constexpr static const i32 flag_w_stack_usage = 77;
constexpr static const i32 flag_w_strict_aliasing = 78;
constexpr static const i32 flag_w_strict_overflow = 79;
constexpr static const i32 flag_w_string_compare = 80;
constexpr static const i32 flag_w_switch = 81;
constexpr static const i32 flag_w_switch_enum = 82;
constexpr static const i32 flag_w_tautological_compare = 83;
constexpr static const i32 flag_w_trampolines = 84;
constexpr static const i32 flag_w_trigraphs = 85;
constexpr static const i32 flag_w_type_limits = 86;
constexpr static const i32 flag_w_undef = 87;
constexpr static const i32 flag_w_uninitialized = 88;
constexpr static const i32 flag_w_unused = 89;
constexpr static const i32 flag_w_unused_parameter = 90;
constexpr static const i32 flag_w_unused_variable = 91;
constexpr static const i32 flag_w_use_after_free = 92;
constexpr static const i32 flag_w_useless_cast = 93;
constexpr static const i32 flag_w_variadic_macros = 94;
constexpr static const i32 flag_w_vla = 95;
constexpr static const i32 flag_w_write_strings = 96;
constexpr static const i32 flag_w_zero_as_null_pointer_constant = 97;
constexpr static const i32 flag_w_no_cpp = 98;

enum class flags : i32 {
  fsyntax_only = flag_fsyntax_only,
  fmax_errors = flag_fmax_errors,
  pedantic = flag_pedantic,
  pedantic_errors = flag_pedantic_errors,
  fpermissive = flag_fpermissive,
  w = flag_w,
  Wextra = flag_w_extra,
  Wall = flag_w_all,
  Wabi = flag_w_abi,
  Waddress = flag_w_address,
  Waggregate_return = flag_w_aggregate_return,
  Walloc_size = flag_w_alloc_size,
  Walloc_zero = flag_w_alloc_zero,
  Walloca = flag_w_alloca,
  Warith_conversion = flag_w_arith_conversion,
  Warray_bounds = flag_w_array_bounds,
  Warray_compare = flag_w_array_compare,
  Warray_parameter = flag_w_array_parameter,
  Wattribute_alias = flag_w_attribute_alias,
  Wbool_compare = flag_w_bool_compare,
  Wbool_operation = flag_w_bool_operation,
  Wc90_c99_compat = flag_w_c90_c99_compat,
  Wc99_c11_compat = flag_w_c99_c11_compat,
  Wc11_c23_compat = flag_w_c11_c23_compat,
  Wc23_c2y_compat = flag_w_c23_c2y_compat,
  Wcxx_compat = flag_w_cxx_compat,
  Wcast_align = flag_w_cast_align,
  Wcast_function_type = flag_w_cast_function_type,
  Wcast_qual = flag_w_cast_qual,
  Wchar_subscripts = flag_w_char_subscripts,
  Wclobbered = flag_w_clobbered,
  Wcomment = flag_w_comment,
  Wconversion = flag_w_conversion,
  Wdangling_pointer = flag_w_dangling_pointer,
  Wdate_time = flag_w_date_time,
  Wdouble_promotion = flag_w_double_promotion,
  Wduplicated_branches = flag_w_duplicated_branches,
  Wduplicated_cond = flag_w_duplicated_cond,
  Wempty_body = flag_w_empty_body,
  Wenum_compare = flag_w_enum_compare,
  Wenum_conversion = flag_w_enum_conversion,
  Werror = flag_w_error,
  Wfatal_errors = flag_w_fatal_errors,
  Wfloat_conversion = flag_w_float_conversion,
  Wfloat_equal = flag_w_float_equal,
  Wformat = flag_w_format,
  Wformat_security = flag_w_format_security,
  Wframe_address = flag_w_frame_address,
  Wheader_guard = flag_w_header_guard,
  Wimplicit_fallthrough = flag_w_implicit_fallthrough,
  Winfinite_recursion = flag_w_infinite_recursion,
  Winit_self = flag_w_init_self,
  Winline = flag_w_inline,
  Wint_in_bool_context = flag_w_int_in_bool_context,
  Winvalid_utf8 = flag_w_invalid_utf8,
  Wjump_misses_init = flag_w_jump_misses_init,
  Wlogical_op = flag_w_logical_op,
  Wlong_long = flag_w_long_long,
  Wmaybe_uninitialized = flag_w_maybe_uninitialized,
  Wmisleading_indentation = flag_w_misleading_indentation,
  Wmissing_braces = flag_w_missing_braces,
  Wmissing_field_initializers = flag_w_missing_field_initializers,
  Wnonnull = flag_w_nonnull,
  Wnull_dereference = flag_w_null_dereference,
  Woverlength_strings = flag_w_overlength_strings,
  Wpacked = flag_w_packed,
  Wpadded = flag_w_padded,
  Wparentheses = flag_w_parentheses,
  Wpointer_arith = flag_w_pointer_arith,
  Wredundant_decls = flag_w_redundant_decls,
  Wrestrict = flag_w_restrict,
  Wreturn_type = flag_w_return_type,
  Wsequence_point = flag_w_sequence_point,
  Wshadow = flag_w_shadow,
  Wsign_compare = flag_w_sign_compare,
  Wsign_conversion = flag_w_sign_conversion,
  Wsizeof_pointer_memaccess = flag_w_sizeof_pointer_memaccess,
  Wstack_usage = flag_w_stack_usage,
  Wstrict_aliasing = flag_w_strict_aliasing,
  Wstrict_overflow = flag_w_strict_overflow,
  Wstring_compare = flag_w_string_compare,
  Wswitch = flag_w_switch,
  Wswitch_enum = flag_w_switch_enum,
  Wtautological_compare = flag_w_tautological_compare,
  Wtrampolines = flag_w_trampolines,
  Wtrigraphs = flag_w_trigraphs,
  Wtype_limits = flag_w_type_limits,
  Wundef = flag_w_undef,
  Wuninitialized = flag_w_uninitialized,
  Wunused = flag_w_unused,
  Wunused_parameter = flag_w_unused_parameter,
  Wunused_variable = flag_w_unused_variable,
  Wuse_after_free = flag_w_use_after_free,
  Wuseless_cast = flag_w_useless_cast,
  Wvariadic_macros = flag_w_variadic_macros,
  Wvla = flag_w_vla,
  Wwrite_strings = flag_w_write_strings,
  Wzero_as_null_pointer_constant = flag_w_zero_as_null_pointer_constant,
  Wno_cpp = flag_w_no_cpp
};

constexpr static const char *flag_strings[] = { "-fsyntax-only",
                                                "-fmax-errors=",
                                                "-Wpedantic",
                                                "-pedantic-errors",
                                                "-fpermissive",
                                                "-w",
                                                "-Wextra",
                                                "-Wall",
                                                "-Wabi=",
                                                "-Waddress",
                                                "-Waggregate-return",
                                                "-Walloc-size",
                                                "-Walloc-zero",
                                                "-Walloca",
                                                "-Warith-conversion",
                                                "-Warray-bounds",
                                                "-Warray-compare",
                                                "-Warray-parameter",
                                                "-Wattribute-alias",
                                                "-Wbool-compare",
                                                "-Wbool-operation",
                                                "-Wc90-c99-compat",
                                                "-Wc99-c11-compat",
                                                "-Wc11-c23-compat",
                                                "-Wc23-c2y-compat",
                                                "-Wc++-compat",
                                                "-Wcast-align",
                                                "-Wcast-function-type",
                                                "-Wcast-qual",
                                                "-Wchar-subscripts",
                                                "-Wclobbered",
                                                "-Wcomment",
                                                "-Wconversion",
                                                "-Wdangling-pointer",
                                                "-Wdate-time",
                                                "-Wdouble-promotion",
                                                "-Wduplicated-branches",
                                                "-Wduplicated-cond",
                                                "-Wempty-body",
                                                "-Wenum-compare",
                                                "-Wenum-conversion",
                                                "-Werror",
                                                "-Wfatal-errors",
                                                "-Wfloat-conversion",
                                                "-Wfloat-equal",
                                                "-Wformat",
                                                "-Wformat-security",
                                                "-Wframe-address",
                                                "-Wheader-guard",
                                                "-Wimplicit-fallthrough",
                                                "-Winfinite-recursion",
                                                "-Winit-self",
                                                "-Winline",
                                                "-Wint-in-bool-context",
                                                "-Winvalid-utf8",
                                                "-Wjump-misses-init",
                                                "-Wlogical-op",
                                                "-Wlong-long",
                                                "-Wmaybe-uninitialized",
                                                "-Wmisleading-indentation",
                                                "-Wmissing-braces",
                                                "-Wmissing-field-initializers",
                                                "-Wnonnull",
                                                "-Wnull-dereference",
                                                "-Woverlength-strings",
                                                "-Wpacked",
                                                "-Wpadded",
                                                "-Wparentheses",
                                                "-Wpointer-arith",
                                                "-Wredundant-decls",
                                                "-Wrestrict",
                                                "-Wreturn-type",
                                                "-Wsequence-point",
                                                "-Wshadow",
                                                "-Wsign-compare",
                                                "-Wsign-conversion",
                                                "-Wsizeof-pointer-memaccess",
                                                "-Wstack-usage=",
                                                "-Wstrict-aliasing",
                                                "-Wstrict-overflow",
                                                "-Wstring-compare",
                                                "-Wswitch",
                                                "-Wswitch-enum",
                                                "-Wtautological-compare",
                                                "-Wtrampolines",
                                                "-Wtrigraphs",
                                                "-Wtype-limits",
                                                "-Wundef",
                                                "-Wuninitialized",
                                                "-Wunused",
                                                "-Wunused-parameter",
                                                "-Wunused-variable",
                                                "-Wuse-after-free",
                                                "-Wuseless-cast",
                                                "-Wvariadic-macros",
                                                "-Wvla",
                                                "-Wwrite-strings",
                                                "-Wzero-as-null-pointer-constant",
                                                "-Wno-cpp" };

constexpr const char *
get_string_flag(flags f)
{
  return flag_strings[static_cast<i32>(f)];
}
}

namespace opt_flags
{
// Optimization flag indices
constexpr static const i32 flag_aggressive_loop_optimizations = 0;
constexpr static const i32 flag_align_functions = 1;
constexpr static const i32 flag_align_jumps = 2;
constexpr static const i32 flag_align_labels = 3;
constexpr static const i32 flag_align_loops = 4;
constexpr static const i32 flag_min_function_alignment = 5;
constexpr static const i32 flag_no_allocation_dce = 6;
constexpr static const i32 flag_allow_store_data_races = 7;
constexpr static const i32 flag_associative_math = 8;
constexpr static const i32 flag_auto_profile = 9;
constexpr static const i32 flag_auto_inc_dec = 10;
constexpr static const i32 flag_branch_probabilities = 11;
constexpr static const i32 flag_caller_saves = 12;
constexpr static const i32 flag_combine_stack_adjustments = 13;
constexpr static const i32 flag_conserve_stack = 14;
constexpr static const i32 flag_fold_mem_offsets = 15;
constexpr static const i32 flag_compare_elim = 16;
constexpr static const i32 flag_cprop_registers = 17;
constexpr static const i32 flag_crossjumping = 18;
constexpr static const i32 flag_cse_follow_jumps = 19;
constexpr static const i32 flag_cse_skip_blocks = 20;
constexpr static const i32 flag_data_sections = 21;
constexpr static const i32 flag_dce = 22;
constexpr static const i32 flag_delete_null_pointer_checks = 23;
constexpr static const i32 flag_devirtualize = 24;
constexpr static const i32 flag_dse = 25;
constexpr static const i32 flag_early_inlining = 26;
constexpr static const i32 flag_expensive_optimizations = 27;
constexpr static const i32 flag_fast_math = 28;
constexpr static const i32 flag_finite_math_only = 29;
constexpr static const i32 flag_float_store = 30;
constexpr static const i32 flag_forward_propagate = 31;
constexpr static const i32 flag_function_sections = 32;
constexpr static const i32 flag_gcse = 33;
constexpr static const i32 flag_hoist_adjacent_loads = 34;
constexpr static const i32 flag_if_conversion = 35;
constexpr static const i32 flag_inline_functions = 36;
constexpr static const i32 flag_inline_small_functions = 37;
constexpr static const i32 flag_ipa_cp = 38;
constexpr static const i32 flag_ipa_sra = 39;
constexpr static const i32 flag_isolate_erroneous_paths_dereference = 40;
constexpr static const i32 flag_ivopts = 41;
constexpr static const i32 flag_keep_inline_functions = 42;
constexpr static const i32 flag_lifetime_dse = 43;
constexpr static const i32 flag_live_range_shrinkage = 44;
constexpr static const i32 flag_loop_interchange = 45;
constexpr static const i32 flag_loop_unroll_and_jam = 46;
constexpr static const i32 flag_lra_remat = 47;
constexpr static const i32 flag_lto = 48;
constexpr static const i32 flag_merge_constants = 49;
constexpr static const i32 flag_modulo_sched = 50;
constexpr static const i32 flag_move_loop_invariants = 51;
constexpr static const i32 flag_omit_frame_pointer = 52;
constexpr static const i32 flag_optimize_sibling_calls = 53;
constexpr static const i32 flag_partial_inlining = 54;
constexpr static const i32 flag_peel_loops = 55;
constexpr static const i32 flag_prefetch_loop_arrays = 56;
constexpr static const i32 flag_profile_use = 57;
constexpr static const i32 flag_reciprocal_math = 58;
constexpr static const i32 flag_rename_registers = 59;
constexpr static const i32 flag_reorder_blocks = 60;
constexpr static const i32 flag_rounding_math = 61;
constexpr static const i32 flag_schedule_insns = 62;
constexpr static const i32 flag_section_anchors = 63;
constexpr static const i32 flag_semantic_interposition = 64;
constexpr static const i32 flag_shrink_wrap = 65;
constexpr static const i32 flag_signaling_nans = 66;
constexpr static const i32 flag_split_loops = 67;
constexpr static const i32 flag_strict_aliasing = 68;
constexpr static const i32 flag_thread_jumps = 69;
constexpr static const i32 flag_tree_vectorize = 70;
constexpr static const i32 flag_trivial_auto_var_init = 71;
constexpr static const i32 flag_unroll_loops = 72;
constexpr static const i32 flag_unsafe_math_optimizations = 73;
constexpr static const i32 flag_unswitch_loops = 74;
constexpr static const i32 flag_vect_cost_model = 75;
constexpr static const i32 flag_whole_program = 76;
constexpr static const i32 flag_zero_call_used_regs = 77;
constexpr static const i32 flag_optimize = 78;
constexpr static const i32 flag_optimize_zero = 79;
constexpr static const i32 flag_optimize_one = 80;
constexpr static const i32 flag_optimize_two = 81;
constexpr static const i32 flag_optimize_three = 82;
constexpr static const i32 flag_optimize_size = 83;
constexpr static const i32 flag_optimize_fast = 84;
constexpr static const i32 flag_optimize_debug = 85;
constexpr static const i32 flag_optimize_z = 86;
constexpr static const i32 flag_param = 87;

enum class flags : i32 {
  aggressive_loop_optimizations = flag_aggressive_loop_optimizations,
  align_functions = flag_align_functions,
  align_jumps = flag_align_jumps,
  align_labels = flag_align_labels,
  align_loops = flag_align_loops,
  min_function_alignment = flag_min_function_alignment,
  no_allocation_dce = flag_no_allocation_dce,
  allow_store_data_races = flag_allow_store_data_races,
  associative_math = flag_associative_math,
  auto_profile = flag_auto_profile,
  auto_inc_dec = flag_auto_inc_dec,
  branch_probabilities = flag_branch_probabilities,
  caller_saves = flag_caller_saves,
  combine_stack_adjustments = flag_combine_stack_adjustments,
  conserve_stack = flag_conserve_stack,
  fold_mem_offsets = flag_fold_mem_offsets,
  compare_elim = flag_compare_elim,
  cprop_registers = flag_cprop_registers,
  crossjumping = flag_crossjumping,
  cse_follow_jumps = flag_cse_follow_jumps,
  cse_skip_blocks = flag_cse_skip_blocks,
  data_sections = flag_data_sections,
  dce = flag_dce,
  delete_null_pointer_checks = flag_delete_null_pointer_checks,
  devirtualize = flag_devirtualize,
  dse = flag_dse,
  early_inlining = flag_early_inlining,
  expensive_optimizations = flag_expensive_optimizations,
  fast_math = flag_fast_math,
  finite_math_only = flag_finite_math_only,
  float_store = flag_float_store,
  forward_propagate = flag_forward_propagate,
  function_sections = flag_function_sections,
  gcse = flag_gcse,
  hoist_adjacent_loads = flag_hoist_adjacent_loads,
  if_conversion = flag_if_conversion,
  inline_functions = flag_inline_functions,
  inline_small_functions = flag_inline_small_functions,
  ipa_cp = flag_ipa_cp,
  ipa_sra = flag_ipa_sra,
  isolate_erroneous_paths_dereference = flag_isolate_erroneous_paths_dereference,
  ivopts = flag_ivopts,
  keep_inline_functions = flag_keep_inline_functions,
  lifetime_dse = flag_lifetime_dse,
  live_range_shrinkage = flag_live_range_shrinkage,
  loop_interchange = flag_loop_interchange,
  loop_unroll_and_jam = flag_loop_unroll_and_jam,
  lra_remat = flag_lra_remat,
  lto = flag_lto,
  merge_constants = flag_merge_constants,
  modulo_sched = flag_modulo_sched,
  move_loop_invariants = flag_move_loop_invariants,
  omit_frame_pointer = flag_omit_frame_pointer,
  optimize_sibling_calls = flag_optimize_sibling_calls,
  partial_inlining = flag_partial_inlining,
  peel_loops = flag_peel_loops,
  prefetch_loop_arrays = flag_prefetch_loop_arrays,
  profile_use = flag_profile_use,
  reciprocal_math = flag_reciprocal_math,
  rename_registers = flag_rename_registers,
  reorder_blocks = flag_reorder_blocks,
  rounding_math = flag_rounding_math,
  schedule_insns = flag_schedule_insns,
  section_anchors = flag_section_anchors,
  semantic_interposition = flag_semantic_interposition,
  shrink_wrap = flag_shrink_wrap,
  signaling_nans = flag_signaling_nans,
  split_loops = flag_split_loops,
  strict_aliasing = flag_strict_aliasing,
  thread_jumps = flag_thread_jumps,
  tree_vectorize = flag_tree_vectorize,
  trivial_auto_var_init = flag_trivial_auto_var_init,
  unroll_loops = flag_unroll_loops,
  unsafe_math_optimizations = flag_unsafe_math_optimizations,
  unswitch_loops = flag_unswitch_loops,
  vect_cost_model = flag_vect_cost_model,
  whole_program = flag_whole_program,
  zero_call_used_regs = flag_zero_call_used_regs,
  optimize = flag_optimize,
  optimize_zero = flag_optimize_zero,
  optimize_one = flag_optimize_one,
  optimize_two = flag_optimize_two,
  optimize_three = flag_optimize_three,
  optimize_size = flag_optimize_size,
  optimize_fast = flag_optimize_fast,
  optimize_debug = flag_optimize_debug,
  optimize_z = flag_optimize_z,
  param = flag_param
};

constexpr static const char *flag_strings[] = { "-faggressive-loop-optimizations",
                                                "-falign-functions=",
                                                "-falign-jumps=",
                                                "-falign-labels=",
                                                "-falign-loops=",
                                                "-fmin-function-alignment=",
                                                "-fno-allocation-dce",
                                                "-fallow-store-data-races",
                                                "-fassociative-math",
                                                "-fauto-profile",
                                                "-fauto-inc-dec",
                                                "-fbranch-probabilities",
                                                "-fcaller-saves",
                                                "-fcombine-stack-adjustments",
                                                "-fconserve-stack",
                                                "-ffold-mem-offsets",
                                                "-fcompare-elim",
                                                "-fcprop-registers",
                                                "-fcrossjumping",
                                                "-fcse-follow-jumps",
                                                "-fcse-skip-blocks",
                                                "-fdata-sections",
                                                "-fdce",
                                                "-fdelete-null-pointer-checks",
                                                "-fdevirtualize",
                                                "-fdse",
                                                "-fearly-inlining",
                                                "-fexpensive-optimizations",
                                                "-ffast-math",
                                                "-ffinite-math-only",
                                                "-ffloat-store",
                                                "-fforward-propagate",
                                                "-ffunction-sections",
                                                "-fgcse",
                                                "-fhoist-adjacent-loads",
                                                "-fif-conversion",
                                                "-finline-functions",
                                                "-finline-small-functions",
                                                "-fipa-cp",
                                                "-fipa-sra",
                                                "-fisolate-erroneous-paths-dereference",
                                                "-fivopts",
                                                "-fkeep-inline-functions",
                                                "-flifetime-dse",
                                                "-flive-range-shrinkage",
                                                "-floop-interchange",
                                                "-floop-unroll-and-jam",
                                                "-flra-remat",
                                                "-flto",
                                                "-fmerge-constants",
                                                "-fmodulo-sched",
                                                "-fmove-loop-invariants",
                                                "-fomit-frame-pointer",
                                                "-foptimize-sibling-calls",
                                                "-fpartial-inlining",
                                                "-fpeel-loops",
                                                "-fprefetch-loop-arrays",
                                                "-fprofile-use",
                                                "-freciprocal-math",
                                                "-frename-registers",
                                                "-freorder-blocks",
                                                "-frounding-math",
                                                "-fschedule-insns",
                                                "-fsection-anchors",
                                                "-fsemantic-interposition",
                                                "-fshrink-wrap",
                                                "-fsignaling-nans",
                                                "-fsplit-loops",
                                                "-fstrict-aliasing",
                                                "-fthread-jumps",
                                                "-ftree-vectorize",
                                                "-ftrivial-auto-var-init",
                                                "-funroll-loops",
                                                "-funsafe-math-optimizations",
                                                "-funswitch-loops",
                                                "-fvect-cost-model",
                                                "-fwhole-program",
                                                "-fzero-call-used-regs",
                                                "-O",
                                                "-O0",
                                                "-O1",
                                                "-O2",
                                                "-O3",
                                                "-Os",
                                                "-Ofast",
                                                "-Og",
                                                "-Oz",
                                                "--param " };

constexpr const char *
get_string_flag(flags f)
{
  return flag_strings[static_cast<i32>(f)];
}
}

namespace analyzer_flags
{
constexpr static const i32 flag_analyzer = 0;
constexpr static const i32 flag_analyzer_call_summaries = 1;
constexpr static const i32 flag_analyzer_checker = 2;
constexpr static const i32 flag_no_analyzer_feasibility = 3;
constexpr static const i32 flag_analyzer_fine_grained = 4;
constexpr static const i32 flag_analyzer_show_events_in_system_headers = 5;
constexpr static const i32 flag_no_analyzer_state_merge = 6;
constexpr static const i32 flag_no_analyzer_state_purge = 7;
constexpr static const i32 flag_no_analyzer_suppress_followups = 8;
constexpr static const i32 flag_analyzer_transitivity = 9;
constexpr static const i32 flag_no_analyzer_undo_inlining = 10;
constexpr static const i32 flag_analyzer_verbose_edges = 11;
constexpr static const i32 flag_analyzer_verbose_state_changes = 12;
constexpr static const i32 flag_analyzer_verbosity = 13;
constexpr static const i32 flag_dump_analyzer = 14;
constexpr static const i32 flag_dump_analyzer_callgraph = 15;
constexpr static const i32 flag_dump_analyzer_exploded_graph = 16;
constexpr static const i32 flag_dump_analyzer_exploded_nodes = 17;
constexpr static const i32 flag_dump_analyzer_exploded_nodes_2 = 18;
constexpr static const i32 flag_dump_analyzer_exploded_nodes_3 = 19;
constexpr static const i32 flag_dump_analyzer_exploded_paths = 20;
constexpr static const i32 flag_dump_analyzer_feasibility = 21;
constexpr static const i32 flag_dump_analyzer_infinite_loop = 22;
constexpr static const i32 flag_dump_analyzer_json = 23;
constexpr static const i32 flag_dump_analyzer_state_purge = 24;
constexpr static const i32 flag_dump_analyzer_stderr = 25;
constexpr static const i32 flag_dump_analyzer_supergraph = 26;
constexpr static const i32 flag_dump_analyzer_untracked = 27;
constexpr static const i32 flag_w_no_analyzer_double_fclose = 28;
constexpr static const i32 flag_w_no_analyzer_double_free = 29;
constexpr static const i32 flag_w_no_analyzer_exposure_through_output_file = 30;
constexpr static const i32 flag_w_no_analyzer_exposure_through_uninit_copy = 31;
constexpr static const i32 flag_w_no_analyzer_fd_access_mode_mismatch = 32;
constexpr static const i32 flag_w_no_analyzer_fd_double_close = 33;
constexpr static const i32 flag_w_no_analyzer_fd_leak = 34;
constexpr static const i32 flag_w_no_analyzer_fd_phase_mismatch = 35;
constexpr static const i32 flag_w_no_analyzer_fd_type_mismatch = 36;
constexpr static const i32 flag_w_no_analyzer_fd_use_after_close = 37;
constexpr static const i32 flag_w_no_analyzer_fd_use_without_check = 38;
constexpr static const i32 flag_w_no_analyzer_file_leak = 39;
constexpr static const i32 flag_w_no_analyzer_free_of_non_heap = 40;
constexpr static const i32 flag_w_no_analyzer_imprecise_fp_arithmetic = 41;
constexpr static const i32 flag_w_no_analyzer_infinite_loop = 42;
constexpr static const i32 flag_w_no_analyzer_infinite_recursion = 43;
constexpr static const i32 flag_w_no_analyzer_jump_through_null = 44;
constexpr static const i32 flag_w_no_analyzer_malloc_leak = 45;
constexpr static const i32 flag_w_no_analyzer_mismatching_deallocation = 46;
constexpr static const i32 flag_w_no_analyzer_null_argument = 47;
constexpr static const i32 flag_w_no_analyzer_null_dereference = 48;
constexpr static const i32 flag_w_no_analyzer_out_of_bounds = 49;
constexpr static const i32 flag_w_no_analyzer_overlapping_buffers = 50;
constexpr static const i32 flag_w_no_analyzer_possible_null_argument = 51;
constexpr static const i32 flag_w_no_analyzer_possible_null_dereference = 52;
constexpr static const i32 flag_w_no_analyzer_putenv_of_auto_var = 53;
constexpr static const i32 flag_w_no_analyzer_shift_count_negative = 54;
constexpr static const i32 flag_w_no_analyzer_shift_count_overflow = 55;
constexpr static const i32 flag_w_no_analyzer_stale_setjmp_buffer = 56;
constexpr static const i32 flag_w_no_analyzer_tainted_allocation_size = 57;
constexpr static const i32 flag_w_no_analyzer_tainted_assertion = 58;
constexpr static const i32 flag_w_no_analyzer_tainted_array_index = 59;
constexpr static const i32 flag_w_no_analyzer_tainted_divisor = 60;
constexpr static const i32 flag_w_no_analyzer_tainted_offset = 61;
constexpr static const i32 flag_w_no_analyzer_tainted_size = 62;
constexpr static const i32 flag_w_analyzer_symbol_too_complex = 63;
constexpr static const i32 flag_w_analyzer_too_complex = 64;
constexpr static const i32 flag_w_no_analyzer_undefined_behavior_ptrdiff = 65;
constexpr static const i32 flag_w_no_analyzer_undefined_behavior_strtok = 66;
constexpr static const i32 flag_w_no_analyzer_unsafe_call_within_signal_handler = 67;
constexpr static const i32 flag_w_no_analyzer_use_after_free = 68;
constexpr static const i32 flag_w_no_analyzer_use_of_pointer_in_stale_stack_frame = 69;
constexpr static const i32 flag_w_no_analyzer_use_of_uninitialized_value = 70;
constexpr static const i32 flag_w_no_analyzer_va_arg_type_mismatch = 71;
constexpr static const i32 flag_w_no_analyzer_va_list_exhausted = 72;
constexpr static const i32 flag_w_no_analyzer_va_list_leak = 73;
constexpr static const i32 flag_w_no_analyzer_va_list_use_after_va_end = 74;
constexpr static const i32 flag_w_no_analyzer_write_to_const = 75;
constexpr static const i32 flag_w_no_analyzer_write_to_string_literal = 76;

enum class flags : i32 {
  analyzer = flag_analyzer,
  analyzer_call_summaries = flag_analyzer_call_summaries,
  analyzer_checker = flag_analyzer_checker,
  no_analyzer_feasibility = flag_no_analyzer_feasibility,
  analyzer_fine_grained = flag_analyzer_fine_grained,
  analyzer_show_events_in_system_headers = flag_analyzer_show_events_in_system_headers,
  no_analyzer_state_merge = flag_no_analyzer_state_merge,
  no_analyzer_state_purge = flag_no_analyzer_state_purge,
  no_analyzer_suppress_followups = flag_no_analyzer_suppress_followups,
  analyzer_transitivity = flag_analyzer_transitivity,
  no_analyzer_undo_inlining = flag_no_analyzer_undo_inlining,
  analyzer_verbose_edges = flag_analyzer_verbose_edges,
  analyzer_verbose_state_changes = flag_analyzer_verbose_state_changes,
  analyzer_verbosity = flag_analyzer_verbosity,
  dump_analyzer = flag_dump_analyzer,
  dump_analyzer_callgraph = flag_dump_analyzer_callgraph,
  dump_analyzer_exploded_graph = flag_dump_analyzer_exploded_graph,
  dump_analyzer_exploded_nodes = flag_dump_analyzer_exploded_nodes,
  dump_analyzer_exploded_nodes_2 = flag_dump_analyzer_exploded_nodes_2,
  dump_analyzer_exploded_nodes_3 = flag_dump_analyzer_exploded_nodes_3,
  dump_analyzer_exploded_paths = flag_dump_analyzer_exploded_paths,
  dump_analyzer_feasibility = flag_dump_analyzer_feasibility,
  dump_analyzer_infinite_loop = flag_dump_analyzer_infinite_loop,
  dump_analyzer_json = flag_dump_analyzer_json,
  dump_analyzer_state_purge = flag_dump_analyzer_state_purge,
  dump_analyzer_stderr = flag_dump_analyzer_stderr,
  dump_analyzer_supergraph = flag_dump_analyzer_supergraph,
  dump_analyzer_untracked = flag_dump_analyzer_untracked,
  w_no_analyzer_double_fclose = flag_w_no_analyzer_double_fclose,
  w_no_analyzer_double_free = flag_w_no_analyzer_double_free,
  w_no_analyzer_exposure_through_output_file = flag_w_no_analyzer_exposure_through_output_file,
  w_no_analyzer_exposure_through_uninit_copy = flag_w_no_analyzer_exposure_through_uninit_copy,
  w_no_analyzer_fd_access_mode_mismatch = flag_w_no_analyzer_fd_access_mode_mismatch,
  w_no_analyzer_fd_double_close = flag_w_no_analyzer_fd_double_close,
  w_no_analyzer_fd_leak = flag_w_no_analyzer_fd_leak,
  w_no_analyzer_fd_phase_mismatch = flag_w_no_analyzer_fd_phase_mismatch,
  w_no_analyzer_fd_type_mismatch = flag_w_no_analyzer_fd_type_mismatch,
  w_no_analyzer_fd_use_after_close = flag_w_no_analyzer_fd_use_after_close,
  w_no_analyzer_fd_use_without_check = flag_w_no_analyzer_fd_use_without_check,
  w_no_analyzer_file_leak = flag_w_no_analyzer_file_leak,
  w_no_analyzer_free_of_non_heap = flag_w_no_analyzer_free_of_non_heap,
  w_no_analyzer_imprecise_fp_arithmetic = flag_w_no_analyzer_imprecise_fp_arithmetic,
  w_no_analyzer_infinite_loop = flag_w_no_analyzer_infinite_loop,
  w_no_analyzer_infinite_recursion = flag_w_no_analyzer_infinite_recursion,
  w_no_analyzer_jump_through_null = flag_w_no_analyzer_jump_through_null,
  w_no_analyzer_malloc_leak = flag_w_no_analyzer_malloc_leak,
  w_no_analyzer_mismatching_deallocation = flag_w_no_analyzer_mismatching_deallocation,
  w_no_analyzer_null_argument = flag_w_no_analyzer_null_argument,
  w_no_analyzer_null_dereference = flag_w_no_analyzer_null_dereference,
  w_no_analyzer_out_of_bounds = flag_w_no_analyzer_out_of_bounds,
  w_no_analyzer_overlapping_buffers = flag_w_no_analyzer_overlapping_buffers,
  w_no_analyzer_possible_null_argument = flag_w_no_analyzer_possible_null_argument,
  w_no_analyzer_possible_null_dereference = flag_w_no_analyzer_possible_null_dereference,
  w_no_analyzer_putenv_of_auto_var = flag_w_no_analyzer_putenv_of_auto_var,
  w_no_analyzer_shift_count_negative = flag_w_no_analyzer_shift_count_negative,
  w_no_analyzer_shift_count_overflow = flag_w_no_analyzer_shift_count_overflow,
  w_no_analyzer_stale_setjmp_buffer = flag_w_no_analyzer_stale_setjmp_buffer,
  w_no_analyzer_tainted_allocation_size = flag_w_no_analyzer_tainted_allocation_size,
  w_no_analyzer_tainted_assertion = flag_w_no_analyzer_tainted_assertion,
  w_no_analyzer_tainted_array_index = flag_w_no_analyzer_tainted_array_index,
  w_no_analyzer_tainted_divisor = flag_w_no_analyzer_tainted_divisor,
  w_no_analyzer_tainted_offset = flag_w_no_analyzer_tainted_offset,
  w_no_analyzer_tainted_size = flag_w_no_analyzer_tainted_size,
  w_analyzer_symbol_too_complex = flag_w_analyzer_symbol_too_complex,
  w_analyzer_too_complex = flag_w_analyzer_too_complex,
  w_no_analyzer_undefined_behavior_ptrdiff = flag_w_no_analyzer_undefined_behavior_ptrdiff,
  w_no_analyzer_undefined_behavior_strtok = flag_w_no_analyzer_undefined_behavior_strtok,
  w_no_analyzer_unsafe_call_within_signal_handler = flag_w_no_analyzer_unsafe_call_within_signal_handler,
  w_no_analyzer_use_after_free = flag_w_no_analyzer_use_after_free,
  w_no_analyzer_use_of_pointer_in_stale_stack_frame = flag_w_no_analyzer_use_of_pointer_in_stale_stack_frame,
  w_no_analyzer_use_of_uninitialized_value = flag_w_no_analyzer_use_of_uninitialized_value,
  w_no_analyzer_va_arg_type_mismatch = flag_w_no_analyzer_va_arg_type_mismatch,
  w_no_analyzer_va_list_exhausted = flag_w_no_analyzer_va_list_exhausted,
  w_no_analyzer_va_list_leak = flag_w_no_analyzer_va_list_leak,
  w_no_analyzer_va_list_use_after_va_end = flag_w_no_analyzer_va_list_use_after_va_end,
  w_no_analyzer_write_to_const = flag_w_no_analyzer_write_to_const,
  w_no_analyzer_write_to_string_literal = flag_w_no_analyzer_write_to_string_literal
};
}

namespace debug_flags
{
constexpr static const i32 flag_g = 0;
constexpr static const i32 flag_glevel = 1;
constexpr static const i32 flag_gdwarf = 2;
constexpr static const i32 flag_gdwarf_version = 3;
constexpr static const i32 flag_gbtf = 4;
constexpr static const i32 flag_gctf = 5;
constexpr static const i32 flag_gctflevel = 6;
constexpr static const i32 flag_gprune_btf = 7;
constexpr static const i32 flag_gno_prune_btf = 8;
constexpr static const i32 flag_ggdb = 9;
constexpr static const i32 flag_grecord_gcc_switches = 10;
constexpr static const i32 flag_gno_record_gcc_switches = 11;
constexpr static const i32 flag_gstrict_dwarf = 12;
constexpr static const i32 flag_gno_strict_dwarf = 13;
constexpr static const i32 flag_gas_loc_support = 14;
constexpr static const i32 flag_gno_as_loc_support = 15;
constexpr static const i32 flag_gas_locview_support = 16;
constexpr static const i32 flag_gno_as_locview_support = 17;
constexpr static const i32 flag_gcodeview = 18;
constexpr static const i32 flag_gcolumn_info = 19;
constexpr static const i32 flag_gno_column_info = 20;
constexpr static const i32 flag_gdwarf32 = 21;
constexpr static const i32 flag_gdwarf64 = 22;
constexpr static const i32 flag_gstatement_frontiers = 23;
constexpr static const i32 flag_gno_statement_frontiers = 24;
constexpr static const i32 flag_gvariable_location_views = 25;
constexpr static const i32 flag_gno_variable_location_views = 26;
constexpr static const i32 flag_ginternal_reset_location_views = 27;
constexpr static const i32 flag_gno_internal_reset_location_views = 28;
constexpr static const i32 flag_ginline_points = 29;
constexpr static const i32 flag_gno_inline_points = 30;
constexpr static const i32 flag_gvms = 31;
constexpr static const i32 flag_gz = 32;
constexpr static const i32 flag_gsplit_dwarf = 33;
constexpr static const i32 flag_gdescribe_dies = 34;
constexpr static const i32 flag_gno_describe_dies = 35;
constexpr static const i32 flag_debug_prefix_map = 36;
constexpr static const i32 flag_debug_types_section = 37;
constexpr static const i32 flag_no_eliminate_unused_debug_types = 38;
constexpr static const i32 flag_emit_struct_debug_baseonly = 39;
constexpr static const i32 flag_emit_struct_debug_reduced = 40;
constexpr static const i32 flag_emit_struct_debug_detailed = 41;
constexpr static const i32 flag_no_eliminate_unused_debug_symbols = 42;
constexpr static const i32 flag_emit_class_debug_always = 43;
constexpr static const i32 flag_no_merge_debug_strings = 44;
constexpr static const i32 flag_no_dwarf2_cfi_asm = 45;
constexpr static const i32 flag_var_tracking = 46;
constexpr static const i32 flag_var_tracking_assignments = 47;

enum class flags : i32 {
  g = flag_g,
  glevel = flag_glevel,
  gdwarf = flag_gdwarf,
  gdwarf_version = flag_gdwarf_version,
  gbtf = flag_gbtf,
  gctf = flag_gctf,
  gctflevel = flag_gctflevel,
  gprune_btf = flag_gprune_btf,
  gno_prune_btf = flag_gno_prune_btf,
  ggdb = flag_ggdb,
  grecord_gcc_switches = flag_grecord_gcc_switches,
  gno_record_gcc_switches = flag_gno_record_gcc_switches,
  gstrict_dwarf = flag_gstrict_dwarf,
  gno_strict_dwarf = flag_gno_strict_dwarf,
  gas_loc_support = flag_gas_loc_support,
  gno_as_loc_support = flag_gno_as_loc_support,
  gas_locview_support = flag_gas_locview_support,
  gno_as_locview_support = flag_gno_as_locview_support,
  gcodeview = flag_gcodeview,
  gcolumn_info = flag_gcolumn_info,
  gno_column_info = flag_gno_column_info,
  gdwarf32 = flag_gdwarf32,
  gdwarf64 = flag_gdwarf64,
  gstatement_frontiers = flag_gstatement_frontiers,
  gno_statement_frontiers = flag_gno_statement_frontiers,
  gvariable_location_views = flag_gvariable_location_views,
  gno_variable_location_views = flag_gno_variable_location_views,
  ginternal_reset_location_views = flag_ginternal_reset_location_views,
  gno_internal_reset_location_views = flag_gno_internal_reset_location_views,
  ginline_points = flag_ginline_points,
  gno_inline_points = flag_gno_inline_points,
  gvms = flag_gvms,
  gz = flag_gz,
  gsplit_dwarf = flag_gsplit_dwarf,
  gdescribe_dies = flag_gdescribe_dies,
  gno_describe_dies = flag_gno_describe_dies,
  debug_prefix_map = flag_debug_prefix_map,
  debug_types_section = flag_debug_types_section,
  no_eliminate_unused_debug_types = flag_no_eliminate_unused_debug_types,
  emit_struct_debug_baseonly = flag_emit_struct_debug_baseonly,
  emit_struct_debug_reduced = flag_emit_struct_debug_reduced,
  emit_struct_debug_detailed = flag_emit_struct_debug_detailed,
  no_eliminate_unused_debug_symbols = flag_no_eliminate_unused_debug_symbols,
  emit_class_debug_always = flag_emit_class_debug_always,
  no_merge_debug_strings = flag_no_merge_debug_strings,
  no_dwarf2_cfi_asm = flag_no_dwarf2_cfi_asm,
  var_tracking = flag_var_tracking,
  var_tracking_assignments = flag_var_tracking_assignments
};

// TODO: finish
constexpr static const char *flag_strings[] = { "-g",
                                                "..." };

constexpr const char *
get_string_flag(flags f)
{
  return flag_strings[static_cast<i32>(f)];
}

}

namespace profiling_flags
{
constexpr static const i32 flag_p = 0;
constexpr static const i32 flag_pg = 1;
constexpr static const i32 flag_profile_arcs = 2;
constexpr static const i32 flag_coverage = 3;
constexpr static const i32 flag_test_coverage = 4;
constexpr static const i32 flag_condition_coverage = 5;
constexpr static const i32 flag_path_coverage = 6;
constexpr static const i32 flag_profile_abs_path = 7;
constexpr static const i32 flag_profile_dir = 8;
constexpr static const i32 flag_profile_generate = 9;
constexpr static const i32 flag_profile_generate_path = 10;
constexpr static const i32 flag_profile_info_section = 11;
constexpr static const i32 flag_profile_info_section_name = 12;
constexpr static const i32 flag_profile_note = 13;
constexpr static const i32 flag_profile_prefix_path = 14;
constexpr static const i32 flag_profile_update = 15;
constexpr static const i32 flag_profile_filter_files = 16;
constexpr static const i32 flag_profile_exclude_files = 17;
constexpr static const i32 flag_profile_reproducible = 18;
constexpr static const i32 flag_sanitize = 19;
constexpr static const i32 flag_sanitize_recover = 20;
constexpr static const i32 flag_sanitize_recover_style = 21;
constexpr static const i32 flag_sanitize_trap = 22;
constexpr static const i32 flag_sanitize_trap_style = 23;
constexpr static const i32 flag_asan_shadow_offset = 24;
constexpr static const i32 flag_sanitize_sections = 25;
constexpr static const i32 flag_sanitize_undefined_trap_on_error = 26;
constexpr static const i32 flag_bounds_check = 27;
constexpr static const i32 flag_cf_protection = 28;
constexpr static const i32 flag_cf_protection_mode = 29;
constexpr static const i32 flag_harden_compares = 30;
constexpr static const i32 flag_harden_conditional_branches = 31;
constexpr static const i32 flag_hardened = 32;
constexpr static const i32 flag_harden_control_flow_redundancy = 33;
constexpr static const i32 flag_hardcfr_skip_leaf = 34;
constexpr static const i32 flag_hardcfr_check_exceptions = 35;
constexpr static const i32 flag_hardcfr_check_returning_calls = 36;
constexpr static const i32 flag_hardcfr_check_noreturn_calls = 37;
constexpr static const i32 flag_stack_protector = 38;
constexpr static const i32 flag_stack_protector_all = 39;
constexpr static const i32 flag_stack_protector_strong = 40;
constexpr static const i32 flag_stack_protector_explicit = 41;
constexpr static const i32 flag_stack_check = 42;
constexpr static const i32 flag_stack_limit_register = 43;
constexpr static const i32 flag_stack_limit_symbol = 44;
constexpr static const i32 flag_no_stack_limit = 45;
constexpr static const i32 flag_split_stack = 46;
constexpr static const i32 flag_strub_disable = 47;
constexpr static const i32 flag_strub_strict = 48;
constexpr static const i32 flag_strub_relaxed = 49;
constexpr static const i32 flag_strub_all = 50;
constexpr static const i32 flag_strub_at_calls = 51;
constexpr static const i32 flag_strub_internal = 52;
constexpr static const i32 flag_vtable_verify = 53;
constexpr static const i32 flag_vtv_counts = 54;
constexpr static const i32 flag_vtv_debug = 55;
constexpr static const i32 flag_instrument_functions = 56;
constexpr static const i32 flag_instrument_functions_once = 57;
constexpr static const i32 flag_instrument_functions_exclude_function_list = 58;
constexpr static const i32 flag_instrument_functions_exclude_file_list = 59;
constexpr static const i32 flag_profile_prefix_map = 60;
constexpr static const i32 flag_patchable_function_entry = 61;

enum class flags : i32 {
  p = flag_p,
  pg = flag_pg,
  profile_arcs = flag_profile_arcs,
  coverage = flag_coverage,
  test_coverage = flag_test_coverage,
  condition_coverage = flag_condition_coverage,
  path_coverage = flag_path_coverage,
  profile_abs_path = flag_profile_abs_path,
  profile_dir = flag_profile_dir,
  profile_generate = flag_profile_generate,
  profile_generate_path = flag_profile_generate_path,
  profile_info_section = flag_profile_info_section,
  profile_info_section_name = flag_profile_info_section_name,
  profile_note = flag_profile_note,
  profile_prefix_path = flag_profile_prefix_path,
  profile_update = flag_profile_update,
  profile_filter_files = flag_profile_filter_files,
  profile_exclude_files = flag_profile_exclude_files,
  profile_reproducible = flag_profile_reproducible,
  sanitize = flag_sanitize,
  sanitize_recover = flag_sanitize_recover,
  sanitize_recover_style = flag_sanitize_recover_style,
  sanitize_trap = flag_sanitize_trap,
  sanitize_trap_style = flag_sanitize_trap_style,
  asan_shadow_offset = flag_asan_shadow_offset,
  sanitize_sections = flag_sanitize_sections,
  sanitize_undefined_trap_on_error = flag_sanitize_undefined_trap_on_error,
  bounds_check = flag_bounds_check,
  cf_protection = flag_cf_protection,
  cf_protection_mode = flag_cf_protection_mode,
  harden_compares = flag_harden_compares,
  harden_conditional_branches = flag_harden_conditional_branches,
  hardened = flag_hardened,
  harden_control_flow_redundancy = flag_harden_control_flow_redundancy,
  hardcfr_skip_leaf = flag_hardcfr_skip_leaf,
  hardcfr_check_exceptions = flag_hardcfr_check_exceptions,
  hardcfr_check_returning_calls = flag_hardcfr_check_returning_calls,
  hardcfr_check_noreturn_calls = flag_hardcfr_check_noreturn_calls,
  stack_protector = flag_stack_protector,
  stack_protector_all = flag_stack_protector_all,
  stack_protector_strong = flag_stack_protector_strong,
  stack_protector_explicit = flag_stack_protector_explicit,
  stack_check = flag_stack_check,
  stack_limit_register = flag_stack_limit_register,
  stack_limit_symbol = flag_stack_limit_symbol,
  no_stack_limit = flag_no_stack_limit,
  split_stack = flag_split_stack,
  strub_disable = flag_strub_disable,
  strub_strict = flag_strub_strict,
  strub_relaxed = flag_strub_relaxed,
  strub_all = flag_strub_all,
  strub_at_calls = flag_strub_at_calls,
  strub_internal = flag_strub_internal,
  vtable_verify = flag_vtable_verify,
  vtv_counts = flag_vtv_counts,
  vtv_debug = flag_vtv_debug,
  instrument_functions = flag_instrument_functions,
  instrument_functions_once = flag_instrument_functions_once,
  instrument_functions_exclude_function_list = flag_instrument_functions_exclude_function_list,
  instrument_functions_exclude_file_list = flag_instrument_functions_exclude_file_list,
  profile_prefix_map = flag_profile_prefix_map,
  patchable_function_entry = flag_patchable_function_entry
};
}

namespace preprocessor_flags
{
constexpr static const i32 flag_a_question = 0;
constexpr static const i32 flag_a_question_answer = 1;
constexpr static const i32 flag_c = 2;
constexpr static const i32 flag_cc = 3;
constexpr static const i32 flag_d_macro = 4;
constexpr static const i32 flag_dd = 5;
constexpr static const i32 flag_di = 6;
constexpr static const i32 flag_dm = 7;
constexpr static const i32 flag_dn = 8;
constexpr static const i32 flag_du = 9;
constexpr static const i32 flag_debug_cpp = 10;
constexpr static const i32 flag_directives_only = 11;
constexpr static const i32 flag_dollars_in_identifiers = 12;
constexpr static const i32 flag_exec_charset = 13;
constexpr static const i32 flag_extended_identifiers = 14;
constexpr static const i32 flag_input_charset = 15;
constexpr static const i32 flag_macro_prefix_map = 16;
constexpr static const i32 flag_max_include_depth = 17;
constexpr static const i32 flag_no_canonical_system_headers = 18;
constexpr static const i32 flag_pch_deps = 19;
constexpr static const i32 flag_pch_preprocess = 20;
constexpr static const i32 flag_preprocessed = 21;
constexpr static const i32 flag_tabstop = 22;
constexpr static const i32 flag_track_macro_expansion = 23;
constexpr static const i32 flag_wide_exec_charset = 24;
constexpr static const i32 flag_working_directory = 25;
constexpr static const i32 flag_h = 26;
constexpr static const i32 flag_imacros = 27;
constexpr static const i32 flag_include = 28;
constexpr static const i32 flag_m = 29;
constexpr static const i32 flag_md = 30;
constexpr static const i32 flag_mf = 31;
constexpr static const i32 flag_mg = 32;
constexpr static const i32 flag_mm = 33;
constexpr static const i32 flag_mmd = 34;
constexpr static const i32 flag_mp = 35;
constexpr static const i32 flag_mq = 36;
constexpr static const i32 flag_mt = 37;
constexpr static const i32 flag_mno_modules = 38;
constexpr static const i32 flag_no_integrated_cpp = 39;
constexpr static const i32 flag_p_preproc = 40;
constexpr static const i32 flag_pthread = 41;
constexpr static const i32 flag_remap = 42;
constexpr static const i32 flag_traditional = 43;
constexpr static const i32 flag_traditional_cpp = 44;
constexpr static const i32 flag_trigraphs = 45;
constexpr static const i32 flag_u_macro = 46;
constexpr static const i32 flag_undef = 47;
constexpr static const i32 flag_wp = 48;
constexpr static const i32 flag_xpreprocessor = 49;

enum class flags : i32 {
  a_question = flag_a_question,
  a_question_answer = flag_a_question_answer,
  c = flag_c,
  cc = flag_cc,
  d_macro = flag_d_macro,
  dd = flag_dd,
  di = flag_di,
  dm = flag_dm,
  dn = flag_dn,
  du = flag_du,
  debug_cpp = flag_debug_cpp,
  directives_only = flag_directives_only,
  dollars_in_identifiers = flag_dollars_in_identifiers,
  exec_charset = flag_exec_charset,
  extended_identifiers = flag_extended_identifiers,
  input_charset = flag_input_charset,
  macro_prefix_map = flag_macro_prefix_map,
  max_include_depth = flag_max_include_depth,
  no_canonical_system_headers = flag_no_canonical_system_headers,
  pch_deps = flag_pch_deps,
  pch_preprocess = flag_pch_preprocess,
  preprocessed = flag_preprocessed,
  tabstop = flag_tabstop,
  track_macro_expansion = flag_track_macro_expansion,
  wide_exec_charset = flag_wide_exec_charset,
  working_directory = flag_working_directory,
  h = flag_h,
  imacros = flag_imacros,
  include = flag_include,
  m = flag_m,
  md = flag_md,
  mf = flag_mf,
  mg = flag_mg,
  mm = flag_mm,
  mmd = flag_mmd,
  mp = flag_mp,
  mq = flag_mq,
  mt = flag_mt,
  mno_modules = flag_mno_modules,
  no_integrated_cpp = flag_no_integrated_cpp,
  p_preproc = flag_p_preproc,
  pthread = flag_pthread,
  remap = flag_remap,
  traditional = flag_traditional,
  traditional_cpp = flag_traditional_cpp,
  trigraphs = flag_trigraphs,
  u_macro = flag_u_macro,
  undef = flag_undef,
  wp = flag_wp,
  xpreprocessor = flag_xpreprocessor
};
}

namespace linker_flags
{
constexpr static const i32 flag_object_file_name = 0;
constexpr static const i32 flag_use_ld = 1;
constexpr static const i32 flag_l_library = 2;
constexpr static const i32 flag_nostartfiles = 3;
constexpr static const i32 flag_nodefaultlibs = 4;
constexpr static const i32 flag_nolibc = 5;
constexpr static const i32 flag_nostdlib = 6;
constexpr static const i32 flag_nostdlib_pp = 7;
constexpr static const i32 flag_e_entry = 8;
constexpr static const i32 flag_entry = 9;
constexpr static const i32 flag_pie = 10;
constexpr static const i32 flag_pthread_link = 11;
constexpr static const i32 flag_r = 12;
constexpr static const i32 flag_rdynamic = 13;
constexpr static const i32 flag_s = 14;
constexpr static const i32 flag_static = 15;
constexpr static const i32 flag_static_pie = 16;
constexpr static const i32 flag_static_libgcc = 17;
constexpr static const i32 flag_static_libstdc_pp = 18;
constexpr static const i32 flag_static_libasan = 19;
constexpr static const i32 flag_static_libtsan = 20;
constexpr static const i32 flag_static_liblsan = 21;
constexpr static const i32 flag_static_libubsan = 22;
constexpr static const i32 flag_shared = 23;
constexpr static const i32 flag_shared_libgcc = 24;
constexpr static const i32 flag_symbolic = 25;
constexpr static const i32 flag_t_script = 26;
constexpr static const i32 flag_wl = 27;
constexpr static const i32 flag_xlinker = 28;
constexpr static const i32 flag_u_symbol = 29;
constexpr static const i32 flag_z_keyword = 30;

enum class flags : i32 {
  object_file_name = flag_object_file_name,
  use_ld = flag_use_ld,
  l_library = flag_l_library,
  nostartfiles = flag_nostartfiles,
  nodefaultlibs = flag_nodefaultlibs,
  nolibc = flag_nolibc,
  nostdlib = flag_nostdlib,
  nostdlib_pp = flag_nostdlib_pp,
  e_entry = flag_e_entry,
  entry = flag_entry,
  pie = flag_pie,
  pthread_link = flag_pthread_link,
  r = flag_r,
  rdynamic = flag_rdynamic,
  s = flag_s,
  static_link = flag_static,
  static_pie = flag_static_pie,
  static_libgcc = flag_static_libgcc,
  static_libstdc_pp = flag_static_libstdc_pp,
  static_libasan = flag_static_libasan,
  static_libtsan = flag_static_libtsan,
  static_liblsan = flag_static_liblsan,
  static_libubsan = flag_static_libubsan,
  shared = flag_shared,
  shared_libgcc = flag_shared_libgcc,
  symbolic = flag_symbolic,
  t_script = flag_t_script,
  wl = flag_wl,
  xlinker = flag_xlinker,
  u_symbol = flag_u_symbol,
  z_keyword = flag_z_keyword
};
}

namespace x86_flags
{
constexpr static const i32 flag_mtune = 0;
constexpr static const i32 flag_march = 1;
constexpr static const i32 flag_mtune_ctrl = 2;
constexpr static const i32 flag_mdump_tune_features = 3;
constexpr static const i32 flag_mno_default = 4;
constexpr static const i32 flag_mfpmath = 5;
constexpr static const i32 flag_masm = 6;
constexpr static const i32 flag_mno_fancy_math_387 = 7;
constexpr static const i32 flag_mno_fp_ret_in_387 = 8;
constexpr static const i32 flag_m80387 = 9;
constexpr static const i32 flag_mhard_float = 10;
constexpr static const i32 flag_msoft_float = 11;
constexpr static const i32 flag_mno_wide_multiply = 12;
constexpr static const i32 flag_mrtd = 13;
constexpr static const i32 flag_malign_double = 14;
constexpr static const i32 flag_mpreferred_stack_boundary = 15;
constexpr static const i32 flag_mincoming_stack_boundary = 16;
constexpr static const i32 flag_mcld = 17;
constexpr static const i32 flag_mcx16 = 18;
constexpr static const i32 flag_msahf = 19;
constexpr static const i32 flag_mmovbe = 20;
constexpr static const i32 flag_mcrc32 = 21;
constexpr static const i32 flag_mmwait = 22;
constexpr static const i32 flag_mrecip = 23;
constexpr static const i32 flag_mrecip_opt = 24;
constexpr static const i32 flag_mvzeroupper = 25;
constexpr static const i32 flag_mprefer_avx128 = 26;
constexpr static const i32 flag_mprefer_vector_width = 27;
constexpr static const i32 flag_mpartial_vector_fp_math = 28;
constexpr static const i32 flag_mmove_max = 29;
constexpr static const i32 flag_mstore_max = 30;
constexpr static const i32 flag_mnoreturn_no_callee_saved_registers = 31;
constexpr static const i32 flag_mmmx = 32;
constexpr static const i32 flag_msse = 33;
constexpr static const i32 flag_msse2 = 34;
constexpr static const i32 flag_msse3 = 35;
constexpr static const i32 flag_mssse3 = 36;
constexpr static const i32 flag_msse4_1 = 37;
constexpr static const i32 flag_msse4_2 = 38;
constexpr static const i32 flag_msse4 = 39;
constexpr static const i32 flag_mavx = 40;
constexpr static const i32 flag_mavx2 = 41;
constexpr static const i32 flag_mavx512f = 42;
constexpr static const i32 flag_mavx512cd = 43;
constexpr static const i32 flag_mavx512vl = 44;
constexpr static const i32 flag_mavx512bw = 45;
constexpr static const i32 flag_mavx512dq = 46;
constexpr static const i32 flag_mavx512ifma = 47;
constexpr static const i32 flag_mavx512vbmi = 48;
constexpr static const i32 flag_msha = 49;
constexpr static const i32 flag_maes = 50;
constexpr static const i32 flag_mpclmul = 51;
constexpr static const i32 flag_mfsgsbase = 52;
constexpr static const i32 flag_mrdrnd = 53;
constexpr static const i32 flag_mf16c = 54;
constexpr static const i32 flag_mfma = 55;
constexpr static const i32 flag_mpconfig = 56;
constexpr static const i32 flag_mwbnoinvd = 57;
constexpr static const i32 flag_mptwrite = 58;
constexpr static const i32 flag_mclflushopt = 59;
constexpr static const i32 flag_mclwb = 60;
constexpr static const i32 flag_mxsavec = 61;
constexpr static const i32 flag_mxsaves = 62;
constexpr static const i32 flag_msse4a = 63;
constexpr static const i32 flag_m3dnow = 64;
constexpr static const i32 flag_m3dnowa = 65;
constexpr static const i32 flag_mpopcnt = 66;
constexpr static const i32 flag_mabm = 67;
constexpr static const i32 flag_mbmi = 68;
constexpr static const i32 flag_mtbm = 69;
constexpr static const i32 flag_mfma4 = 70;
constexpr static const i32 flag_mxop = 71;
constexpr static const i32 flag_madx = 72;
constexpr static const i32 flag_mlzcnt = 73;
constexpr static const i32 flag_mbmi2 = 74;
constexpr static const i32 flag_mfxsr = 75;
constexpr static const i32 flag_mxsave = 76;
constexpr static const i32 flag_mxsaveopt = 77;
constexpr static const i32 flag_mrtm = 78;
constexpr static const i32 flag_mhle = 79;
constexpr static const i32 flag_mlwp = 80;
constexpr static const i32 flag_mmwaitx = 81;
constexpr static const i32 flag_mclzero = 82;
constexpr static const i32 flag_mpku = 83;
constexpr static const i32 flag_mthreads = 84;
constexpr static const i32 flag_mgfni = 85;
constexpr static const i32 flag_mvaes = 86;
constexpr static const i32 flag_mwaitpkg = 87;
constexpr static const i32 flag_mshstk = 88;
constexpr static const i32 flag_mmanual_endbr = 89;
constexpr static const i32 flag_mcet_switch = 90;
constexpr static const i32 flag_mforce_indirect_call = 91;
constexpr static const i32 flag_mavx512vbmi2 = 92;
constexpr static const i32 flag_mavx512bf16 = 93;
constexpr static const i32 flag_menqcmd = 94;
constexpr static const i32 flag_mvpclmulqdq = 95;
constexpr static const i32 flag_mavx512bitalg = 96;
constexpr static const i32 flag_mmovdiri = 97;
constexpr static const i32 flag_mmovdir64b = 98;
constexpr static const i32 flag_mavx512vpopcntdq = 99;
constexpr static const i32 flag_mavx512vnni = 100;
constexpr static const i32 flag_mprfchw = 101;
constexpr static const i32 flag_mrdpid = 102;
constexpr static const i32 flag_mrdseed = 103;
constexpr static const i32 flag_msgx = 104;
constexpr static const i32 flag_mavx512vp2intersect = 105;
constexpr static const i32 flag_mserialize = 106;
constexpr static const i32 flag_mtsxldtrk = 107;
constexpr static const i32 flag_mamx_tile = 108;
constexpr static const i32 flag_mamx_int8 = 109;
constexpr static const i32 flag_mamx_bf16 = 110;
constexpr static const i32 flag_muintr = 111;
constexpr static const i32 flag_mhreset = 112;
constexpr static const i32 flag_mavxvnni = 113;
constexpr static const i32 flag_mamx_fp8 = 114;
constexpr static const i32 flag_mavx512fp16 = 115;
constexpr static const i32 flag_mavxifma = 116;
constexpr static const i32 flag_mavxvnniint8 = 117;
constexpr static const i32 flag_mavxneconvert = 118;
constexpr static const i32 flag_mcmpccxadd = 119;
constexpr static const i32 flag_mamx_fp16 = 120;
constexpr static const i32 flag_mprefetchi = 121;
constexpr static const i32 flag_mraoint = 122;
constexpr static const i32 flag_mamx_complex = 123;
constexpr static const i32 flag_mavxvnniint16 = 124;
constexpr static const i32 flag_msm3 = 125;
constexpr static const i32 flag_msha512 = 126;
constexpr static const i32 flag_msm4 = 127;
constexpr static const i32 flag_mapxf = 128;
constexpr static const i32 flag_musermsr = 129;
constexpr static const i32 flag_mavx10_1 = 130;
constexpr static const i32 flag_mavx10_1_256 = 131;
constexpr static const i32 flag_mavx10_1_512 = 132;
constexpr static const i32 flag_mevex512 = 133;
constexpr static const i32 flag_mavx10_2 = 134;
constexpr static const i32 flag_mamx_avx512 = 135;
constexpr static const i32 flag_mamx_tf32 = 136;
constexpr static const i32 flag_mamx_transpose = 137;
constexpr static const i32 flag_mmovrs = 138;
constexpr static const i32 flag_mamx_movrs = 139;
constexpr static const i32 flag_mcldemote = 140;
constexpr static const i32 flag_mms_bitfields = 141;
constexpr static const i32 flag_mno_align_stringops = 142;
constexpr static const i32 flag_minline_all_stringops = 143;
constexpr static const i32 flag_minline_stringops_dynamically = 144;
constexpr static const i32 flag_mstringop_strategy = 145;
constexpr static const i32 flag_mkl = 146;
constexpr static const i32 flag_mwidekl = 147;
constexpr static const i32 flag_mmemcpy_strategy = 148;
constexpr static const i32 flag_mmemset_strategy = 149;
constexpr static const i32 flag_mpush_args = 150;
constexpr static const i32 flag_maccumulate_outgoing_args = 151;
constexpr static const i32 flag_m128bit_long_double = 152;
constexpr static const i32 flag_m96bit_long_double = 153;
constexpr static const i32 flag_mlong_double_64 = 154;
constexpr static const i32 flag_mlong_double_80 = 155;
constexpr static const i32 flag_mlong_double_128 = 156;
constexpr static const i32 flag_mregparm = 157;
constexpr static const i32 flag_msseregparm = 158;
constexpr static const i32 flag_mveclibabi = 159;
constexpr static const i32 flag_mvect8_ret_in_mem = 160;
constexpr static const i32 flag_mpc32 = 161;
constexpr static const i32 flag_mpc64 = 162;
constexpr static const i32 flag_mpc80 = 163;
constexpr static const i32 flag_mdaz_ftz = 164;
constexpr static const i32 flag_mstackrealign = 165;
constexpr static const i32 flag_momit_leaf_frame_pointer = 166;
constexpr static const i32 flag_mno_red_zone = 167;
constexpr static const i32 flag_mno_tls_direct_seg_refs = 168;
constexpr static const i32 flag_mcmodel = 169;
constexpr static const i32 flag_mabi = 170;
constexpr static const i32 flag_maddress_mode = 171;
constexpr static const i32 flag_m32 = 172;
constexpr static const i32 flag_m64 = 173;
constexpr static const i32 flag_mx32 = 174;
constexpr static const i32 flag_m16 = 175;
constexpr static const i32 flag_miamcu = 176;
constexpr static const i32 flag_mlarge_data_threshold = 177;
constexpr static const i32 flag_msse2avx = 178;
constexpr static const i32 flag_mfentry = 179;
constexpr static const i32 flag_mrecord_mcount = 180;
constexpr static const i32 flag_mnop_mcount = 181;
constexpr static const i32 flag_m8bit_idiv = 182;
constexpr static const i32 flag_minstrument_return = 183;
constexpr static const i32 flag_mfentry_name = 184;
constexpr static const i32 flag_mfentry_section = 185;
constexpr static const i32 flag_mavx256_split_unaligned_load = 186;
constexpr static const i32 flag_mavx256_split_unaligned_store = 187;
constexpr static const i32 flag_malign_data = 188;
constexpr static const i32 flag_mstack_protector_guard = 189;
constexpr static const i32 flag_mstack_protector_guard_reg = 190;
constexpr static const i32 flag_mstack_protector_guard_offset = 191;
constexpr static const i32 flag_mstack_protector_guard_symbol = 192;
constexpr static const i32 flag_mgeneral_regs_only = 193;
constexpr static const i32 flag_mcall_ms2sysv_xlogues = 194;
constexpr static const i32 flag_mrelax_cmpxchg_loop = 195;
constexpr static const i32 flag_mindirect_branch = 196;
constexpr static const i32 flag_mfunction_return = 197;
constexpr static const i32 flag_mindirect_branch_register = 198;
constexpr static const i32 flag_mharden_sls = 199;
constexpr static const i32 flag_mindirect_branch_cs_prefix = 200;
constexpr static const i32 flag_mneeded = 201;
constexpr static const i32 flag_mno_direct_extern_access = 202;
constexpr static const i32 flag_munroll_only_small_loops = 203;
constexpr static const i32 flag_mlam = 204;
constexpr static const i32 flag_march_native = 205;

enum class flags : i32 {
  mtune = flag_mtune,
  march = flag_march,
  mtune_ctrl = flag_mtune_ctrl,
  mdump_tune_features = flag_mdump_tune_features,
  mno_default = flag_mno_default,
  mfpmath = flag_mfpmath,
  masm = flag_masm,
  mno_fancy_math_387 = flag_mno_fancy_math_387,
  mno_fp_ret_in_387 = flag_mno_fp_ret_in_387,
  m80387 = flag_m80387,
  mhard_float = flag_mhard_float,
  msoft_float = flag_msoft_float,
  mno_wide_multiply = flag_mno_wide_multiply,
  mrtd = flag_mrtd,
  malign_double = flag_malign_double,
  mpreferred_stack_boundary = flag_mpreferred_stack_boundary,
  mincoming_stack_boundary = flag_mincoming_stack_boundary,
  mcld = flag_mcld,
  mcx16 = flag_mcx16,
  msahf = flag_msahf,
  mmovbe = flag_mmovbe,
  mcrc32 = flag_mcrc32,
  mmwait = flag_mmwait,
  mrecip = flag_mrecip,
  mrecip_opt = flag_mrecip_opt,
  mvzeroupper = flag_mvzeroupper,
  mprefer_avx128 = flag_mprefer_avx128,
  mprefer_vector_width = flag_mprefer_vector_width,
  mpartial_vector_fp_math = flag_mpartial_vector_fp_math,
  mmove_max = flag_mmove_max,
  mstore_max = flag_mstore_max,
  mnoreturn_no_callee_saved_registers = flag_mnoreturn_no_callee_saved_registers,
  mmmx = flag_mmmx,
  msse = flag_msse,
  msse2 = flag_msse2,
  msse3 = flag_msse3,
  mssse3 = flag_mssse3,
  msse4_1 = flag_msse4_1,
  msse4_2 = flag_msse4_2,
  msse4 = flag_msse4,
  mavx = flag_mavx,
  mavx2 = flag_mavx2,
  mavx512f = flag_mavx512f,
  mavx512cd = flag_mavx512cd,
  mavx512vl = flag_mavx512vl,
  mavx512bw = flag_mavx512bw,
  mavx512dq = flag_mavx512dq,
  mavx512ifma = flag_mavx512ifma,
  mavx512vbmi = flag_mavx512vbmi,
  msha = flag_msha,
  maes = flag_maes,
  mpclmul = flag_mpclmul,
  mfsgsbase = flag_mfsgsbase,
  mrdrnd = flag_mrdrnd,
  mf16c = flag_mf16c,
  mfma = flag_mfma,
  mpconfig = flag_mpconfig,
  mwbnoinvd = flag_mwbnoinvd,
  mptwrite = flag_mptwrite,
  mclflushopt = flag_mclflushopt,
  mclwb = flag_mclwb,
  mxsavec = flag_mxsavec,
  mxsaves = flag_mxsaves,
  msse4a = flag_msse4a,
  m3dnow = flag_m3dnow,
  m3dnowa = flag_m3dnowa,
  mpopcnt = flag_mpopcnt,
  mabm = flag_mabm,
  mbmi = flag_mbmi,
  mtbm = flag_mtbm,
  mfma4 = flag_mfma4,
  mxop = flag_mxop,
  madx = flag_madx,
  mlzcnt = flag_mlzcnt,
  mbmi2 = flag_mbmi2,
  mfxsr = flag_mfxsr,
  mxsave = flag_mxsave,
  mxsaveopt = flag_mxsaveopt,
  mrtm = flag_mrtm,
  mhle = flag_mhle,
  mlwp = flag_mlwp,
  mmwaitx = flag_mmwaitx,
  mclzero = flag_mclzero,
  mpku = flag_mpku,
  mthreads = flag_mthreads,
  mgfni = flag_mgfni,
  mvaes = flag_mvaes,
  mwaitpkg = flag_mwaitpkg,
  mshstk = flag_mshstk,
  mmanual_endbr = flag_mmanual_endbr,
  mcet_switch = flag_mcet_switch,
  mforce_indirect_call = flag_mforce_indirect_call,
  mavx512vbmi2 = flag_mavx512vbmi2,
  mavx512bf16 = flag_mavx512bf16,
  menqcmd = flag_menqcmd,
  mvpclmulqdq = flag_mvpclmulqdq,
  mavx512bitalg = flag_mavx512bitalg,
  mmovdiri = flag_mmovdiri,
  mmovdir64b = flag_mmovdir64b,
  mavx512vpopcntdq = flag_mavx512vpopcntdq,
  mavx512vnni = flag_mavx512vnni,
  mprfchw = flag_mprfchw,
  mrdpid = flag_mrdpid,
  mrdseed = flag_mrdseed,
  msgx = flag_msgx,
  mavx512vp2intersect = flag_mavx512vp2intersect,
  mserialize = flag_mserialize,
  mtsxldtrk = flag_mtsxldtrk,
  mamx_tile = flag_mamx_tile,
  mamx_int8 = flag_mamx_int8,
  mamx_bf16 = flag_mamx_bf16,
  muintr = flag_muintr,
  mhreset = flag_mhreset,
  mavxvnni = flag_mavxvnni,
  mamx_fp8 = flag_mamx_fp8,
  mavx512fp16 = flag_mavx512fp16,
  mavxifma = flag_mavxifma,
  mavxvnniint8 = flag_mavxvnniint8,
  mavxneconvert = flag_mavxneconvert,
  mcmpccxadd = flag_mcmpccxadd,
  mamx_fp16 = flag_mamx_fp16,
  mprefetchi = flag_mprefetchi,
  mraoint = flag_mraoint,
  mamx_complex = flag_mamx_complex,
  mavxvnniint16 = flag_mavxvnniint16,
  msm3 = flag_msm3,
  msha512 = flag_msha512,
  msm4 = flag_msm4,
  mapxf = flag_mapxf,
  musermsr = flag_musermsr,
  mavx10_1 = flag_mavx10_1,
  mavx10_1_256 = flag_mavx10_1_256,
  mavx10_1_512 = flag_mavx10_1_512,
  mevex512 = flag_mevex512,
  mavx10_2 = flag_mavx10_2,
  mamx_avx512 = flag_mamx_avx512,
  mamx_tf32 = flag_mamx_tf32,
  mamx_transpose = flag_mamx_transpose,
  mmovrs = flag_mmovrs,
  mamx_movrs = flag_mamx_movrs,
  mcldemote = flag_mcldemote,
  mms_bitfields = flag_mms_bitfields,
  mno_align_stringops = flag_mno_align_stringops,
  minline_all_stringops = flag_minline_all_stringops,
  minline_stringops_dynamically = flag_minline_stringops_dynamically,
  mstringop_strategy = flag_mstringop_strategy,
  mkl = flag_mkl,
  mwidekl = flag_mwidekl,
  mmemcpy_strategy = flag_mmemcpy_strategy,
  mmemset_strategy = flag_mmemset_strategy,
  mpush_args = flag_mpush_args,
  maccumulate_outgoing_args = flag_maccumulate_outgoing_args,
  m128bit_long_double = flag_m128bit_long_double,
  m96bit_long_double = flag_m96bit_long_double,
  mlong_double_64 = flag_mlong_double_64,
  mlong_double_80 = flag_mlong_double_80,
  mlong_double_128 = flag_mlong_double_128,
  mregparm = flag_mregparm,
  msseregparm = flag_msseregparm,
  mveclibabi = flag_mveclibabi,
  mvect8_ret_in_mem = flag_mvect8_ret_in_mem,
  mpc32 = flag_mpc32,
  mpc64 = flag_mpc64,
  mpc80 = flag_mpc80,
  mdaz_ftz = flag_mdaz_ftz,
  mstackrealign = flag_mstackrealign,
  momit_leaf_frame_pointer = flag_momit_leaf_frame_pointer,
  mno_red_zone = flag_mno_red_zone,
  mno_tls_direct_seg_refs = flag_mno_tls_direct_seg_refs,
  mcmodel = flag_mcmodel,
  mabi = flag_mabi,
  maddress_mode = flag_maddress_mode,
  m32 = flag_m32,
  m64 = flag_m64,
  mx32 = flag_mx32,
  m16 = flag_m16,
  miamcu = flag_miamcu,
  mlarge_data_threshold = flag_mlarge_data_threshold,
  msse2avx = flag_msse2avx,
  mfentry = flag_mfentry,
  mrecord_mcount = flag_mrecord_mcount,
  mnop_mcount = flag_mnop_mcount,
  m8bit_idiv = flag_m8bit_idiv,
  minstrument_return = flag_minstrument_return,
  mfentry_name = flag_mfentry_name,
  mfentry_section = flag_mfentry_section,
  mavx256_split_unaligned_load = flag_mavx256_split_unaligned_load,
  mavx256_split_unaligned_store = flag_mavx256_split_unaligned_store,
  malign_data = flag_malign_data,
  mstack_protector_guard = flag_mstack_protector_guard,
  mstack_protector_guard_reg = flag_mstack_protector_guard_reg,
  mstack_protector_guard_offset = flag_mstack_protector_guard_offset,
  mstack_protector_guard_symbol = flag_mstack_protector_guard_symbol,
  mgeneral_regs_only = flag_mgeneral_regs_only,
  mcall_ms2sysv_xlogues = flag_mcall_ms2sysv_xlogues,
  mrelax_cmpxchg_loop = flag_mrelax_cmpxchg_loop,
  mindirect_branch = flag_mindirect_branch,
  mfunction_return = flag_mfunction_return,
  mindirect_branch_register = flag_mindirect_branch_register,
  mharden_sls = flag_mharden_sls,
  mindirect_branch_cs_prefix = flag_mindirect_branch_cs_prefix,
  mneeded = flag_mneeded,
  mno_direct_extern_access = flag_mno_direct_extern_access,
  munroll_only_small_loops = flag_munroll_only_small_loops,
  mlam = flag_mlam,
  march_native = flag_march_native
};

constexpr static const char *flag_strings[] = { "-mtune=",
                                                "-march=",
                                                "-mtune-ctrl=",
                                                "-mdump-tune-features",
                                                "-mno-default",
                                                "-mfpmath=",
                                                "-masm=",
                                                "-mno-fancy-math-387",
                                                "-mno-fp-ret-in-387",
                                                "-m80387",
                                                "-mhard-float",
                                                "-msoft-float",
                                                "-mno-wide-multiply",
                                                "-mrtd",
                                                "-malign-double",
                                                "-mpreferred-stack-boundary=",
                                                "-mincoming-stack-boundary=",
                                                "-mcld",
                                                "-mcx16",
                                                "-msahf",
                                                "-mmovbe",
                                                "-mcrc32",
                                                "-mmwait",
                                                "-mrecip",
                                                "-mrecip=",
                                                "-mvzeroupper",
                                                "-mprefer-avx128",
                                                "-mprefer-vector-width=",
                                                "-mpartial-vector-fp-math",
                                                "-mmove-max=",
                                                "-mstore-max=",
                                                "-mnoreturn-no-callee-saved-registers",
                                                "-mmmx",
                                                "-msse",
                                                "-msse2",
                                                "-msse3",
                                                "-mssse3",
                                                "-msse4.1",
                                                "-msse4.2",
                                                "-msse4",
                                                "-mavx",
                                                "-mavx2",
                                                "-mavx512f",
                                                "-mavx512cd",
                                                "-mavx512vl",
                                                "-mavx512bw",
                                                "-mavx512dq",
                                                "-mavx512ifma",
                                                "-mavx512vbmi",
                                                "-msha",
                                                "-maes",
                                                "-mpclmul",
                                                "-mfsgsbase",
                                                "-mrdrnd",
                                                "-mf16c",
                                                "-mfma",
                                                "-mpconfig",
                                                "-mwbnoinvd",
                                                "-mptwrite",
                                                "-mclflushopt",
                                                "-mclwb",
                                                "-mxsavec",
                                                "-mxsaves",
                                                "-msse4a",
                                                "-m3dnow",
                                                "-m3dnowa",
                                                "-mpopcnt",
                                                "-mabm",
                                                "-mbmi",
                                                "-mtbm",
                                                "-mfma4",
                                                "-mxop",
                                                "-madx",
                                                "-mlzcnt",
                                                "-mbmi2",
                                                "-mfxsr",
                                                "-mxsave",
                                                "-mxsaveopt",
                                                "-mrtm",
                                                "-mhle",
                                                "-mlwp",
                                                "-mmwaitx",
                                                "-mclzero",
                                                "-mpku",
                                                "-mthreads",
                                                "-mgfni",
                                                "-mvaes",
                                                "-mwaitpkg",
                                                "-mshstk",
                                                "-mmanual-endbr",
                                                "-mcet-switch",
                                                "-mforce-indirect-call",
                                                "-mavx512vbmi2",
                                                "-mavx512bf16",
                                                "-menqcmd",
                                                "-mvpclmulqdq",
                                                "-mavx512bitalg",
                                                "-mmovdiri",
                                                "-mmovdir64b",
                                                "-mavx512vpopcntdq",
                                                "-mavx512vnni",
                                                "-mprfchw",
                                                "-mrdpid",
                                                "-mrdseed",
                                                "-msgx",
                                                "-mavx512vp2intersect",
                                                "-mserialize",
                                                "-mtsxldtrk",
                                                "-mamx-tile",
                                                "-mamx-int8",
                                                "-mamx-bf16",
                                                "-muintr",
                                                "-mhreset",
                                                "-mavxvnni",
                                                "-mamx-fp8",
                                                "-mavx512fp16",
                                                "-mavxifma",
                                                "-mavxvnniint8",
                                                "-mavxneconvert",
                                                "-mcmpccxadd",
                                                "-mamx-fp16",
                                                "-mprefetchi",
                                                "-mraoint",
                                                "-mamx-complex",
                                                "-mavxvnniint16",
                                                "-msm3",
                                                "-msha512",
                                                "-msm4",
                                                "-mapxf",
                                                "-musermsr",
                                                "-mavx10.1",
                                                "-mavx10.1-256",
                                                "-mavx10.1-512",
                                                "-mevex512",
                                                "-mavx10.2",
                                                "-mamx-avx512",
                                                "-mamx-tf32",
                                                "-mamx-transpose",
                                                "-mmovrs",
                                                "-mamx-movrs",
                                                "-mcldemote",
                                                "-mms-bitfields",
                                                "-mno-align-stringops",
                                                "-minline-all-stringops",
                                                "-minline-stringops-dynamically",
                                                "-mstringop-strategy=",
                                                "-mkl",
                                                "-mwidekl",
                                                "-mmemcpy-strategy=",
                                                "-mmemset-strategy=",
                                                "-mpush-args",
                                                "-maccumulate-outgoing-args",
                                                "-m128bit-long-double",
                                                "-m96bit-long-double",
                                                "-mlong-double-64",
                                                "-mlong-double-80",
                                                "-mlong-double-128",
                                                "-mregparm=",
                                                "-msseregparm",
                                                "-mveclibabi=",
                                                "-mvect8-ret-in-mem",
                                                "-mpc32",
                                                "-mpc64",
                                                "-mpc80",
                                                "-mdaz-ftz",
                                                "-mstackrealign",
                                                "-momit-leaf-frame-pointer",
                                                "-mno-red-zone",
                                                "-mno-tls-direct-seg-refs",
                                                "-mcmodel=",
                                                "-mabi=",
                                                "-maddress-mode=",
                                                "-m32",
                                                "-m64",
                                                "-mx32",
                                                "-m16",
                                                "-miamcu",
                                                "-mlarge-data-threshold=",
                                                "-msse2avx",
                                                "-mfentry",
                                                "-mrecord-mcount",
                                                "-mnop-mcount",
                                                "-m8bit-idiv",
                                                "-minstrument-return=",
                                                "-mfentry-name=",
                                                "-mfentry-section=",
                                                "-mavx256-split-unaligned-load",
                                                "-mavx256-split-unaligned-store",
                                                "-malign-data=",
                                                "-mstack-protector-guard=",
                                                "-mstack-protector-guard-reg=",
                                                "-mstack-protector-guard-offset=",
                                                "-mstack-protector-guard-symbol=",
                                                "-mgeneral-regs-only",
                                                "-mcall-ms2sysv-xlogues",
                                                "-mrelax-cmpxchg-loop",
                                                "-mindirect-branch=",
                                                "-mfunction-return=",
                                                "-mindirect-branch-register",
                                                "-mharden-sls=",
                                                "-mindirect-branch-cs-prefix",
                                                "-mneeded",
                                                "-mno-direct-extern-access",
                                                "-munroll-only-small-loops",
                                                "-mlam=",
                                                "-march=native" };

constexpr const char *
get_string_flag(flags f)
{
  return flag_strings[static_cast<i32>(f)];
}
}

namespace aarch64_flags
{
constexpr static const i32 flag_mabi = 0;
constexpr static const i32 flag_mbig_endian = 1;
constexpr static const i32 flag_mlittle_endian = 2;
constexpr static const i32 flag_mgeneral_regs_only = 3;
constexpr static const i32 flag_mcmodel_tiny = 4;
constexpr static const i32 flag_mcmodel_small = 5;
constexpr static const i32 flag_mcmodel_large = 6;
constexpr static const i32 flag_mstrict_align = 7;
constexpr static const i32 flag_mno_strict_align = 8;
constexpr static const i32 flag_momit_leaf_frame_pointer = 9;
constexpr static const i32 flag_mtls_dialect_desc = 10;
constexpr static const i32 flag_mtls_dialect_traditional = 11;
constexpr static const i32 flag_mtls_size = 12;
constexpr static const i32 flag_mfix_cortex_a53_835769 = 13;
constexpr static const i32 flag_mfix_cortex_a53_843419 = 14;
constexpr static const i32 flag_mlow_precision_recip_sqrt = 15;
constexpr static const i32 flag_mlow_precision_sqrt = 16;
constexpr static const i32 flag_mlow_precision_div = 17;
constexpr static const i32 flag_mpc_relative_literal_loads = 18;
constexpr static const i32 flag_msign_return_address = 19;
constexpr static const i32 flag_mbranch_protection = 20;
constexpr static const i32 flag_mharden_sls = 21;
constexpr static const i32 flag_march = 22;
constexpr static const i32 flag_mcpu = 23;
constexpr static const i32 flag_mtune = 24;
constexpr static const i32 flag_moverride = 25;
constexpr static const i32 flag_mverbose_cost_dump = 26;
constexpr static const i32 flag_mstack_protector_guard = 27;
constexpr static const i32 flag_mstack_protector_guard_reg = 28;
constexpr static const i32 flag_mstack_protector_guard_offset = 29;
constexpr static const i32 flag_mtrack_speculation = 30;
constexpr static const i32 flag_moutline_atomics = 31;
constexpr static const i32 flag_mearly_ldp_fusion = 32;
constexpr static const i32 flag_mlate_ldp_fusion = 33;
constexpr static const i32 flag_w_experimental_fmv_target = 34;

enum class flags : i32 {
  mabi = flag_mabi,
  mbig_endian = flag_mbig_endian,
  mlittle_endian = flag_mlittle_endian,
  mgeneral_regs_only = flag_mgeneral_regs_only,
  mcmodel_tiny = flag_mcmodel_tiny,
  mcmodel_small = flag_mcmodel_small,
  mcmodel_large = flag_mcmodel_large,
  mstrict_align = flag_mstrict_align,
  mno_strict_align = flag_mno_strict_align,
  momit_leaf_frame_pointer = flag_momit_leaf_frame_pointer,
  mtls_dialect_desc = flag_mtls_dialect_desc,
  mtls_dialect_traditional = flag_mtls_dialect_traditional,
  mtls_size = flag_mtls_size,
  mfix_cortex_a53_835769 = flag_mfix_cortex_a53_835769,
  mfix_cortex_a53_843419 = flag_mfix_cortex_a53_843419,
  mlow_precision_recip_sqrt = flag_mlow_precision_recip_sqrt,
  mlow_precision_sqrt = flag_mlow_precision_sqrt,
  mlow_precision_div = flag_mlow_precision_div,
  mpc_relative_literal_loads = flag_mpc_relative_literal_loads,
  msign_return_address = flag_msign_return_address,
  mbranch_protection = flag_mbranch_protection,
  mharden_sls = flag_mharden_sls,
  march = flag_march,
  mcpu = flag_mcpu,
  mtune = flag_mtune,
  moverride = flag_moverride,
  mverbose_cost_dump = flag_mverbose_cost_dump,
  mstack_protector_guard = flag_mstack_protector_guard,
  mstack_protector_guard_reg = flag_mstack_protector_guard_reg,
  mstack_protector_guard_offset = flag_mstack_protector_guard_offset,
  mtrack_speculation = flag_mtrack_speculation,
  moutline_atomics = flag_moutline_atomics,
  mearly_ldp_fusion = flag_mearly_ldp_fusion,
  mlate_ldp_fusion = flag_mlate_ldp_fusion,
  w_experimental_fmv_target = flag_w_experimental_fmv_target
};

constexpr static const char *flag_strings[] = { "-mabi=",
                                                "-mbig-endian",
                                                "-mlittle-endian",
                                                "-mgeneral-regs-only",
                                                "-mcmodel=tiny",
                                                "-mcmodel=small",
                                                "-mcmodel=large",
                                                "-mstrict-align",
                                                "-mno-strict-align",
                                                "-momit-leaf-frame-pointer",
                                                "-mtls-dialect=desc",
                                                "-mtls-dialect=traditional",
                                                "-mtls-size=",
                                                "-mfix-cortex-a53-835769",
                                                "-mfix-cortex-a53-843419",
                                                "-mlow-precision-recip-sqrt",
                                                "-mlow-precision-sqrt",
                                                "-mlow-precision-div",
                                                "-mpc-relative-literal-loads",
                                                "-msign-return-address=",
                                                "-mbranch-protection=",
                                                "-mharden-sls=",
                                                "-march=",
                                                "-mcpu=",
                                                "-mtune=",
                                                "-moverride=",
                                                "-mverbose-cost-dump",
                                                "-mstack-protector-guard=",
                                                "-mstack-protector-guard-reg=",
                                                "-mstack-protector-guard-offset=",
                                                "-mtrack-speculation",
                                                "-moutline-atomics",
                                                "-mearly-ldp-fusion",
                                                "-mlate-ldp-fusion",
                                                "-Wexperimental-fmv-target" };

constexpr const char *
get_string_flag(flags f)
{
  return flag_strings[static_cast<i32>(f)];
}
}

namespace arm_flags
{
constexpr static const i32 flag_mapcs_frame = 0;
constexpr static const i32 flag_mno_apcs_frame = 1;
constexpr static const i32 flag_mabi = 2;
constexpr static const i32 flag_mapcs_stack_check = 3;
constexpr static const i32 flag_mno_apcs_stack_check = 4;
constexpr static const i32 flag_mapcs_reentrant = 5;
constexpr static const i32 flag_mno_apcs_reentrant = 6;
constexpr static const i32 flag_mgeneral_regs_only = 7;
constexpr static const i32 flag_msched_prolog = 8;
constexpr static const i32 flag_mno_sched_prolog = 9;
constexpr static const i32 flag_mlittle_endian = 10;
constexpr static const i32 flag_mbig_endian = 11;
constexpr static const i32 flag_mbe8 = 12;
constexpr static const i32 flag_mbe32 = 13;
constexpr static const i32 flag_mfloat_abi = 14;
constexpr static const i32 flag_mfp16_format = 15;
constexpr static const i32 flag_mthumb_interwork = 16;
constexpr static const i32 flag_mno_thumb_interwork = 17;
constexpr static const i32 flag_mcpu = 18;
constexpr static const i32 flag_march = 19;
constexpr static const i32 flag_mfpu = 20;
constexpr static const i32 flag_mtune = 21;
constexpr static const i32 flag_mprint_tune_info = 22;
constexpr static const i32 flag_mstructure_size_boundary = 23;
constexpr static const i32 flag_mabort_on_noreturn = 24;
constexpr static const i32 flag_mlong_calls = 25;
constexpr static const i32 flag_mno_long_calls = 26;
constexpr static const i32 flag_msingle_pic_base = 27;
constexpr static const i32 flag_mno_single_pic_base = 28;
constexpr static const i32 flag_mpic_register = 29;
constexpr static const i32 flag_mnop_fun_dllimport = 30;
constexpr static const i32 flag_mpoke_function_name = 31;
constexpr static const i32 flag_mthumb = 32;
constexpr static const i32 flag_marm = 33;
constexpr static const i32 flag_mflip_thumb = 34;
constexpr static const i32 flag_mtpcs_frame = 35;
constexpr static const i32 flag_mtpcs_leaf_frame = 36;
constexpr static const i32 flag_mcaller_super_interworking = 37;
constexpr static const i32 flag_mcallee_super_interworking = 38;
constexpr static const i32 flag_mtp = 39;
constexpr static const i32 flag_mtls_dialect = 40;
constexpr static const i32 flag_mword_relocations = 41;
constexpr static const i32 flag_mfix_cortex_m3_ldrd = 42;
constexpr static const i32 flag_mfix_cortex_a57_aes_1742098 = 43;
constexpr static const i32 flag_mfix_cortex_a72_aes_1655431 = 44;
constexpr static const i32 flag_munaligned_access = 45;
constexpr static const i32 flag_mneon_for_64bits = 46;
constexpr static const i32 flag_mslow_flash_data = 47;
constexpr static const i32 flag_masm_syntax_unified = 48;
constexpr static const i32 flag_mrestrict_it = 49;
constexpr static const i32 flag_mverbose_cost_dump = 50;
constexpr static const i32 flag_mpure_code = 51;
constexpr static const i32 flag_mcmse = 52;
constexpr static const i32 flag_mfix_cmse_cve_2021_35465 = 53;
constexpr static const i32 flag_mstack_protector_guard = 54;
constexpr static const i32 flag_mstack_protector_guard_offset = 55;
constexpr static const i32 flag_mfdpic = 56;
constexpr static const i32 flag_mbranch_protection = 57;

enum class flags : i32 {
  mapcs_frame = flag_mapcs_frame,
  mno_apcs_frame = flag_mno_apcs_frame,
  mabi = flag_mabi,
  mapcs_stack_check = flag_mapcs_stack_check,
  mno_apcs_stack_check = flag_mno_apcs_stack_check,
  mapcs_reentrant = flag_mapcs_reentrant,
  mno_apcs_reentrant = flag_mno_apcs_reentrant,
  mgeneral_regs_only = flag_mgeneral_regs_only,
  msched_prolog = flag_msched_prolog,
  mno_sched_prolog = flag_mno_sched_prolog,
  mlittle_endian = flag_mlittle_endian,
  mbig_endian = flag_mbig_endian,
  mbe8 = flag_mbe8,
  mbe32 = flag_mbe32,
  mfloat_abi = flag_mfloat_abi,
  mfp16_format = flag_mfp16_format,
  mthumb_interwork = flag_mthumb_interwork,
  mno_thumb_interwork = flag_mno_thumb_interwork,
  mcpu = flag_mcpu,
  march = flag_march,
  mfpu = flag_mfpu,
  mtune = flag_mtune,
  mprint_tune_info = flag_mprint_tune_info,
  mstructure_size_boundary = flag_mstructure_size_boundary,
  mabort_on_noreturn = flag_mabort_on_noreturn,
  mlong_calls = flag_mlong_calls,
  mno_long_calls = flag_mno_long_calls,
  msingle_pic_base = flag_msingle_pic_base,
  mno_single_pic_base = flag_mno_single_pic_base,
  mpic_register = flag_mpic_register,
  mnop_fun_dllimport = flag_mnop_fun_dllimport,
  mpoke_function_name = flag_mpoke_function_name,
  mthumb = flag_mthumb,
  marm = flag_marm,
  mflip_thumb = flag_mflip_thumb,
  mtpcs_frame = flag_mtpcs_frame,
  mtpcs_leaf_frame = flag_mtpcs_leaf_frame,
  mcaller_super_interworking = flag_mcaller_super_interworking,
  mcallee_super_interworking = flag_mcallee_super_interworking,
  mtp = flag_mtp,
  mtls_dialect = flag_mtls_dialect,
  mword_relocations = flag_mword_relocations,
  mfix_cortex_m3_ldrd = flag_mfix_cortex_m3_ldrd,
  mfix_cortex_a57_aes_1742098 = flag_mfix_cortex_a57_aes_1742098,
  mfix_cortex_a72_aes_1655431 = flag_mfix_cortex_a72_aes_1655431,
  munaligned_access = flag_munaligned_access,
  mneon_for_64bits = flag_mneon_for_64bits,
  mslow_flash_data = flag_mslow_flash_data,
  masm_syntax_unified = flag_masm_syntax_unified,
  mrestrict_it = flag_mrestrict_it,
  mverbose_cost_dump = flag_mverbose_cost_dump,
  mpure_code = flag_mpure_code,
  mcmse = flag_mcmse,
  mfix_cmse_cve_2021_35465 = flag_mfix_cmse_cve_2021_35465,
  mstack_protector_guard = flag_mstack_protector_guard,
  mstack_protector_guard_offset = flag_mstack_protector_guard_offset,
  mfdpic = flag_mfdpic,
  mbranch_protection = flag_mbranch_protection
};

constexpr static const char *flag_strings[] = { "-mapcs-frame",
                                                "-mno-apcs-frame",
                                                "-mabi=",
                                                "-mapcs-stack-check",
                                                "-mno-apcs-stack-check",
                                                "-mapcs-reentrant",
                                                "-mno-apcs-reentrant",
                                                "-mgeneral-regs-only",
                                                "-msched-prolog",
                                                "-mno-sched-prolog",
                                                "-mlittle-endian",
                                                "-mbig-endian",
                                                "-mbe8",
                                                "-mbe32",
                                                "-mfloat-abi=",
                                                "-mfp16-format=",
                                                "-mthumb-interwork",
                                                "-mno-thumb-interwork",
                                                "-mcpu=",
                                                "-march=",
                                                "-mfpu=",
                                                "-mtune=",
                                                "-mprint-tune-info",
                                                "-mstructure-size-boundary=",
                                                "-mabort-on-noreturn",
                                                "-mlong-calls",
                                                "-mno-long-calls",
                                                "-msingle-pic-base",
                                                "-mno-single-pic-base",
                                                "-mpic-register=",
                                                "-mnop-fun-dllimport",
                                                "-mpoke-function-name",
                                                "-mthumb",
                                                "-marm",
                                                "-mflip-thumb",
                                                "-mtpcs-frame",
                                                "-mtpcs-leaf-frame",
                                                "-mcaller-super-interworking",
                                                "-mcallee-super-interworking",
                                                "-mtp=",
                                                "-mtls-dialect=",
                                                "-mword-relocations",
                                                "-mfix-cortex-m3-ldrd",
                                                "-mfix-cortex-a57-aes-1742098",
                                                "-mfix-cortex-a72-aes-1655431",
                                                "-munaligned-access",
                                                "-mneon-for-64bits",
                                                "-mslow-flash-data",
                                                "-masm-syntax-unified",
                                                "-mrestrict-it",
                                                "-mverbose-cost-dump",
                                                "-mpure-code",
                                                "-mcmse",
                                                "-mfix-cmse-cve-2021-35465",
                                                "-mstack-protector-guard=",
                                                "-mstack-protector-guard-offset=",
                                                "-mfdpic",
                                                "-mbranch-protection=" };

constexpr const char *
get_string_flag(flags f)
{
  return flag_strings[static_cast<i32>(f)];
}
}

}     // namespace gcc
