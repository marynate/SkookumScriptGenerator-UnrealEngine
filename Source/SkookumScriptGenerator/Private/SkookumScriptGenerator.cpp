//=======================================================================================
// SkookumScript Unreal Engine Binding Generator
// Copyright (c) 2015 Agog Labs Inc. All rights reserved.
//
// Author: Markus Breyer
//
// Adapted in parts from sample code by Robert Manuszewski of Epic Games Inc.
//=======================================================================================

#include "SkookumScriptGeneratorPrivatePCH.h"

//#define USE_DEBUG_LOG_FILE

DEFINE_LOG_CATEGORY(LogSkookumScriptGenerator);

class FSkookumScriptGenerator : public ISkookumScriptGenerator
  {

  //---------------------------------------------------------------------------------------
  // IModuleInterface implementation

  virtual void    StartupModule() override;
  virtual void    ShutdownModule() override;

  //---------------------------------------------------------------------------------------
  // IScriptGeneratorPluginInterface implementation

  virtual FString GetGeneratedCodeModuleName() const override { return TEXT("SkookumScriptRuntime"); }
  virtual bool    ShouldExportClassesForModule(const FString & module_name, EBuildModuleType::Type module_type, const FString & module_generated_include_folder) const;
  virtual bool    SupportsTarget(const FString & target_name) const override { return true; }
  virtual void    Initialize(const FString & root_local_path, const FString & root_build_path, const FString & output_directory, const FString & include_base) override;
  virtual void    ExportClass(UClass * class_p, const FString & source_header_file_name, const FString & generated_header_file_name, bool has_changed) override;
  virtual void    FinishExport() override;
  virtual FString GetGeneratorName() const override;

  //---------------------------------------------------------------------------------------
  // Types

  enum eSkTypeID
    {
    SkTypeID_None = 0,
    SkTypeID_Integer,
    SkTypeID_Real,
    SkTypeID_Boolean,
    SkTypeID_String,
    SkTypeID_Name,
    SkTypeID_Vector2,
    SkTypeID_Vector3,
    SkTypeID_Vector4,
    SkTypeID_Rotation,
    SkTypeID_RotationAngles,
    SkTypeID_Transform,
    SkTypeID_Color,
    SkTypeID_Enum,
    SkTypeID_UStruct,
    SkTypeID_UClass,
    SkTypeID_UObject,
    SkTypeID_List,

    SkTypeID__Count
    };

  // To keep track of bindings generated for a particular class
  struct MethodBinding
    {
    void make_method(UFunction * function_p); // create names for a method
    void make_property_getter(UProperty * property_p);
    void make_property_setter(UProperty * property_p);

    bool operator == (const MethodBinding & other) const { return m_script_name == other.m_script_name; }

    FString   m_script_name;
    FString   m_code_name;
    };

  // To keep track of classes for later exporting
  struct ClassRecord
    {
    ClassRecord(UClass * class_p, const FString & source_header_file_name) : m_class_p(class_p), m_source_header_file_name(source_header_file_name) {}

    bool operator == (const ClassRecord & other) const { return m_class_p == other.m_class_p; }

    UClass *  m_class_p;
    FString   m_source_header_file_name;
    };

  // To keep track of structs for later exporting
  struct StructRecord
    {
    StructRecord(UStruct * struct_p, const FString & source_header_file_name) : m_struct_p(struct_p), m_source_header_file_name(source_header_file_name) {}

    bool operator == (const StructRecord & other) const { return m_struct_p == other.m_struct_p; }

    UStruct *  m_struct_p;
    FString    m_source_header_file_name;
    };

  //---------------------------------------------------------------------------------------
  // Data

  static const FString  ms_sk_type_id_names[SkTypeID__Count]; // Names belonging to the ids above
  static const FString  ms_reserved_keywords[]; // = Forbidden variable names
  static const FName                                ms_meta_data_key_function_category;
  static const FFileHelper::EEncodingOptions::Type  ms_script_file_encoding;

  FString               m_binding_code_path; // Output folder for generated binding code files
  FString               m_scripts_path; // Output folder for generated script files
  int32                 m_scripts_path_depth; // Amount of super classes until we start flattening the script file hierarchy due to the evil reign of Windows MAX_PATH. 1 = everything is right under 'Object', 0 is not allowed
  FString               m_unreal_engine_root_path_local; // Root of "Unreal Engine" folder on local machine
  FString               m_unreal_engine_root_path_build; // Root of "Unreal Engine" folder for builds - may be different to m_unreal_engine_root_local if we're building remotely
  FString               m_runtime_plugin_root_path; // Root of the runtime plugin we're generating the code for - used as base path for include files

  TArray<FString>       m_all_header_file_names; // Keep track of all headers generated
  TArray<FString>       m_all_binding_file_names; // Keep track of all binding files generated
  TArray<FString>       m_temp_headers; // Keep track of temp headers generated by save_header_if_changed()
  FString               m_source_header_file_name; // Keep track of source header file passed in

  TSet<UStruct *>       m_exported_classes; // Whenever a class or struct gets exported, it gets added to this list
  TArray<ClassRecord>   m_extra_classes; // Classes rejected to export at first, but possibly exported later if ever used
  TArray<UClass *>      m_used_classes; // All classes used as types (by parameters, properties etc.)
  TArray<FString>       m_skip_classes; // All classes set to skip in UHT config file (Engine/Programs/UnrealHeaderTool/Config/DefaultEngine.ini)

  TSet<UEnum *>         m_exported_enums;

#ifdef USE_DEBUG_LOG_FILE
  FILE *                m_debug_log_file; // Quick file handle to print debug stuff to, generates log file in output folder
#endif

  //---------------------------------------------------------------------------------------
  // Methods

  void                  generate_class(UClass * class_p, const FString & source_header_file_name); // Generate script and binding files for a class and its methods and properties
  void                  generate_class_script_files(UClass * class_p); // Generate script files for a class and its methods and properties 
  void                  generate_class_header_file(UClass * class_p, const FString & source_header_file_name); // Generate header file for a class
  void                  generate_class_binding_file(UClass * class_p); // Generate binding code source file for a class

  void                  generate_struct_from_property(UProperty * prop_p); // Generate script and binding files for a struct from property
  void                  generate_struct(UStruct * struct_p, const FString & source_header_file_name); // // Generate script and binding files for a struct
  void                  generate_struct_script_files(UStruct * struct_p); // Generate script files for a struct
  void                  generate_struct_header_file(UStruct * struct_p, const FString & source_header_file_name); // Generate header file for a struct
  void                  generate_struct_binding_file(UStruct * struct_p); // Generate binding code source file for a struct

  void                  generate_enum_from_property(UProperty * prop_p); // Generate script files for an enum
  void                  generate_enum_script_files(UEnum * enum_p);

  FString               generate_method(const FString & class_name_cpp, UClass * class_p, UFunction * function_p, const MethodBinding & binding); // Generate script file and binding code for a method
  void                  generate_method_script_file(UFunction * function_p, const FString & script_function_name); // Generate script file for a method
  FString               generate_method_binding_code(const FString & class_name_cpp, UClass * class_p, UFunction * function_p, const MethodBinding & binding); // Generate binding code for a method

  FString               generate_property_getter(const FString & class_name_cpp, UStruct * class_or_struct_p, UProperty * property_p, int32 property_index, const MethodBinding & binding); // Generate getter script file and binding code for a property
  FString               generate_property_setter(const FString & class_name_cpp, UStruct * class_or_struct_p, UProperty * property_p, int32 property_index, const MethodBinding & binding); // Generate setter script file and binding code for a property
  void                  generate_property_getter_script_file(UProperty * property_p, const FString & script_function_name); // Generate getter script file for a property
  void                  generate_property_setter_script_file(UProperty * property_p, const FString & script_function_name); // Generate setter script file for a property
  FString               generate_property_getter_binding_code(UProperty * property_p, const FString & class_name_cpp, UStruct * class_or_struct_p, const MethodBinding & binding); // Generate getter binding code for a property
  FString               generate_property_setter_binding_code(UProperty * property_p, const FString & class_name_cpp, UStruct * class_or_struct_p, const MethodBinding & binding); // Generate setter binding code for a property

  FString               generate_method_binding_declaration(const FString & function_name, bool is_static); // Generate declaration of method binding function
  FString               generate_this_pointer_initialization(const FString & class_name_cpp, UStruct * class_or_struct_p, bool is_static); // Generate code that obtains the 'this' pointer from scope_p
  FString               generate_method_parameter_expression(UFunction * function_p, UProperty * param_p, int32 ParamIndex, FString format_string);
  FString               generate_method_out_parameter_expression(UFunction * function_p, UProperty * param_p, int32 ParamIndex, const FString & param_name);
  FString               generate_property_default_ctor_argument(UProperty * param_p);

  FString               generate_return_or_param_value_passing(UStruct * class_or_struct_p, UFunction * function_p, UProperty * return_value_p, const FString & return_value_name, const FString & format_string, int32 ParamIndex = -1); // Generate code that passes back the return value

  void                  generate_master_binding_file(); // Generate master source file that includes all others

  bool                  can_export_class(UClass * class_p, const FString & source_header_file_name) const;
  bool                  can_export_method(UClass * class_p, UFunction * function_p);
  bool                  can_export_property(UStruct * class_or_struct_p, UProperty * property_p);
  bool                  can_export_struct(UStruct * struct_p);
  bool                  is_struct_type_supported(UStruct * struct_p);
  static bool           does_class_have_static_class(UClass * class_p);
  bool                  is_property_type_supported(UProperty * property_p);

  static FString        skookify_class_name(const FString & name);
  static FString        skookify_var_name(const FString & name, bool * is_boolean_p = nullptr);
  static FString        skookify_method_name(const FString & name, UProperty * return_property_p = nullptr);
  static FString        get_skookum_class_name(UStruct * class_or_struct_p);
  FString               get_skookum_class_path(UStruct * class_or_struct_p);
  FString               get_skookum_method_path(UStruct * class_or_struct_p, const FString & script_function_name, bool is_static);
  eSkTypeID             get_skookum_property_type(UProperty * property_p);
  FString               get_skookum_property_type_name(UProperty * property_p);
  static uint32         get_skookum_string_crc(const FString & string);
  FString               get_cpp_class_name(UStruct * class_or_struct_p);
  static FString        get_cpp_property_type_name(UProperty * property_p, uint32 port_flags = 0);
  static FString        get_comment_block(UField * field_p);
  static FString        get_skookum_default_initializer(UFunction * function_p, UProperty * param_p);
  FString               get_array_parameter_expression(UProperty * param_p, int32 ParamIndex);

  bool                  save_header_if_changed(const FString & HeaderPath, const FString & new_header_contents); // Helper to change a file only if needed
  void                  rename_temp_files(); // Puts generated files into place after all code generation is done

  };

IMPLEMENT_MODULE(FSkookumScriptGenerator, SkookumScriptGenerator)

//=======================================================================================
// IModuleInterface implementation
//=======================================================================================

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::StartupModule()
  {
  IModularFeatures::Get().RegisterModularFeature(TEXT("ScriptGenerator"), this);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::ShutdownModule()
  {
  IModularFeatures::Get().UnregisterModularFeature(TEXT("ScriptGenerator"), this);
  }

//=======================================================================================
// IScriptGeneratorPluginInterface implementation
//=======================================================================================

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::Initialize(const FString & root_local_path, const FString & root_build_path, const FString & output_directory, const FString & include_base)
  {
  m_binding_code_path = output_directory;
  m_unreal_engine_root_path_local = root_local_path;
  m_unreal_engine_root_path_build = root_build_path;
  m_runtime_plugin_root_path = include_base;
  m_scripts_path = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*(include_base / TEXT("../../Scripts/Engine-Generated")));

  // Clear contents of scripts folder for a fresh start
  FString directory_to_delete(m_scripts_path / TEXT("Object"));
  IFileManager::Get().DeleteDirectory(*directory_to_delete, false, true);

  // Try to figure the path depth from ini file
  m_scripts_path_depth = 4; // Set to sensible default in case we don't find it in the ini file
  FString ini_file_text;
  if (FFileHelper::LoadFileToString(ini_file_text, *(m_scripts_path / TEXT("../Skookum-project-default.ini"))))
    {
    FRegexPattern regex(TEXT("Overlay[0-9]+=Engine-Generated\\|.*?\\|([0-9]+)"));
    FRegexMatcher matcher(regex, ini_file_text);
    if (matcher.FindNext())
      {
      int32 begin_idx = matcher.GetCaptureGroupBeginning(1);
      if (begin_idx >= 0)
        {
        int32 path_depth = FCString::Atoi(&ini_file_text[begin_idx]);
        if (path_depth > 0)
          {
          m_scripts_path_depth = path_depth;
          }
        }
      }
    }

  // Fetch from ini file which classes to skip during script generation
  // [SkookumScriptGenerator]
  // +SkipClasses=ClassName1
  // +SkipClasses=ClassName2
  GConfig->GetArray(TEXT("SkookumScriptGenerator"), TEXT("SkipClasses"), m_skip_classes, GEngineIni);

  // Create debug log file
#ifdef USE_DEBUG_LOG_FILE
  m_debug_log_file = _wfopen(*(output_directory / TEXT("SkookumScriptGenerator.log.txt")), TEXT("w"));
#endif
  }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::ShouldExportClassesForModule(const FString & module_name, EBuildModuleType::Type module_type, const FString & module_generated_include_folder) const
  {
  bool can_export = (module_type == EBuildModuleType::Runtime || module_type == EBuildModuleType::Game);
  if (can_export)
    {
    // Only export functions from selected modules
    static struct FSupportedModules
      {
      TArray<FString> supported_script_modules;
      FSupportedModules()
        {
        GConfig->GetArray(TEXT("Plugins"), TEXT("ScriptSupportedModules"), supported_script_modules, GEngineIni);
        }
      } supported_modules;

    can_export = supported_modules.supported_script_modules.Num() == 0 || supported_modules.supported_script_modules.Contains(module_name);
    }
  return can_export;
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::ExportClass(UClass * class_p, const FString & source_header_file_name, const FString & generated_header_file_name, bool has_changed)
  {
  // $Revisit MBreyer - (for now) skip and forget classes coming from engine plugins
  if (source_header_file_name.Find(TEXT("Engine/Plugins")) >= 0
    || source_header_file_name.Find(TEXT("Engine\\Plugins")) >= 0)
    {
    return;
    }

  m_source_header_file_name = source_header_file_name;

  if (!can_export_class(class_p, source_header_file_name))
    {
    m_extra_classes.AddUnique(ClassRecord(class_p, source_header_file_name));
    return;
    }

  generate_class(class_p, source_header_file_name);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::FinishExport()
  {
  // Generate any classes that have been used but not exported yet
  for (auto & extra_class : m_extra_classes)
    {
    // Generate it if it's been used anywhere
    bool generate_it = (m_used_classes.Find(extra_class.m_class_p) >= 0);

    // Or, if it's derived from any class already exported
    /*
    if (!generate_it)
    {
    for (UClass * class_p = extra_class.m_class_p; class_p && class_p != UObject::StaticClass(); class_p = class_p->GetSuperClass())
    {
    if (m_exported_classes.Contains(class_p))
    {
    generate_it = true;
    break;
    }
    }
    }
    */

    if (generate_it)
      {
      generate_class(extra_class.m_class_p, extra_class.m_source_header_file_name);
      }
    }

  generate_master_binding_file();
  rename_temp_files();

#ifdef USE_DEBUG_LOG_FILE
  fclose(m_debug_log_file);
#endif
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::GetGeneratorName() const
 {
 return TEXT("SkookumScript Binding Generator Plugin");
 }

//=======================================================================================		//=======================================================================================
// FSkookumScriptGenerator implementation
//=======================================================================================

const FString FSkookumScriptGenerator::ms_sk_type_id_names[FSkookumScriptGenerator::SkTypeID__Count] =
  {
  TEXT("nil"),
  TEXT("Integer"),
  TEXT("Real"),
  TEXT("Boolean"),
  TEXT("String"),
  TEXT("Name"),
  TEXT("Vector2"),
  TEXT("Vector3"),
  TEXT("Vector4"),
  TEXT("Rotation"),
  TEXT("RotationAngles"),
  TEXT("Transform"),
  TEXT("Color"),
  TEXT("Enum"),
  TEXT("UStruct"),
  TEXT("EntityClass"),  // UClass
  TEXT("Entity"),       // UObject
  TEXT("List"),
  };

const FString FSkookumScriptGenerator::ms_reserved_keywords[] =
  {
  TEXT("branch"),
  TEXT("case"),
  TEXT("divert"),
  TEXT("else"),
  TEXT("exit"),
  TEXT("false"),
  TEXT("fork"),
  TEXT("if"),
  TEXT("loop"),
  TEXT("nil"),
  TEXT("race"),
  TEXT("rush"),
  TEXT("skip"),
  TEXT("sync"),
  TEXT("this"),
  TEXT("this_class"),
  TEXT("this_code"),
  TEXT("true"),
  TEXT("unless"),
  TEXT("when"),

  // Boolean word operators
  TEXT("and"),
  TEXT("nand"),
  TEXT("nor"),
  TEXT("not"),
  TEXT("nxor"),
  TEXT("or"),
  TEXT("xor"),
  };

const FName FSkookumScriptGenerator::ms_meta_data_key_function_category(TEXT("Category"));

const FFileHelper::EEncodingOptions::Type FSkookumScriptGenerator::ms_script_file_encoding = FFileHelper::EEncodingOptions::ForceAnsi;

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_class(UClass * class_p, const FString & source_header_file_name)
  {
  UE_LOG(LogSkookumScriptGenerator, Log, TEXT("Generating class %s"), *get_skookum_class_name(class_p));

  m_exported_classes.Add(class_p);

  // Generate script files
  generate_class_script_files(class_p);

  // Generate binding code files	
  generate_class_header_file(class_p, source_header_file_name);
  generate_class_binding_file(class_p);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_class_script_files(UClass * class_p)
  {
  // Create class meta file:
  FString meta_file_path = get_skookum_class_path(class_p) / TEXT("!Class.sk-meta");
  FString body = get_comment_block(class_p);
  if (!FFileHelper::SaveStringToFile(body, *meta_file_path, ms_script_file_encoding))
    {
    FError::Throwf(TEXT("Could not save file: %s"), *meta_file_path);
    }
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_class_header_file(UClass * class_p, const FString & source_header_file_name)
  {
  const FString class_header_file_name = FString::Printf(TEXT("SkUE%s.generated.hpp"), *get_skookum_class_name(class_p));
  m_all_header_file_names.Add(class_header_file_name);

  FString skookum_class_name = get_skookum_class_name(class_p);
  FString cpp_class_name = get_cpp_class_name(class_p);

  FString generated_code;

  generated_code += TEXT("#pragma once\r\n\r\n");
  generated_code += TEXT("#include <Bindings/SkUEClassBinding.hpp>\r\n");

  // if not defined let's hope it is already known when the compiler gets here
  if (source_header_file_name.Len() > 0)
    {
    FString relative_path(source_header_file_name);
    FPaths::MakePathRelativeTo(relative_path, *m_runtime_plugin_root_path);
    generated_code += FString::Printf(TEXT("#include <%s>\r\n\r\n"), *relative_path);
    }
  else
    {
    generated_code += FString::Printf(TEXT("// Note: Include path for %s was unknown at code generation time, so hopefully the class is already known when compilation gets here\r\n\r\n"), *cpp_class_name);
    }

  generated_code += FString::Printf(TEXT("class SkUE%s : public SkUEClassBinding%s<SkUE%s, %s>\r\n  {\r\n"),
    *skookum_class_name, 
    class_p->HasAnyCastFlag(CASTCLASS_AActor) ? TEXT("Actor") : TEXT("Entity"),
    *skookum_class_name, 
    *cpp_class_name);

  generated_code += TEXT("  public:\r\n");
  generated_code += TEXT("    static void register_bindings();\r\n");
  generated_code += TEXT("  };\r\n");

  save_header_if_changed(m_binding_code_path / class_header_file_name, generated_code);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_class_binding_file(UClass * class_p)
  {
  const FString skookum_class_name = get_skookum_class_name(class_p);
  const FString class_binding_file_name = FString::Printf(TEXT("SkUE%s.generated.inl"), *skookum_class_name);
  m_all_binding_file_names.Add(class_binding_file_name);

  const FString class_name_cpp = get_cpp_class_name(class_p);

  enum eScope { Scope_instance, Scope_class }; // 0 = instance, 1 = static bindings
  TArray<MethodBinding> bindings[2]; // eScope
  MethodBinding binding;

  FString generated_code;
  generated_code += FString::Printf(TEXT("\r\nnamespace SkUE%s_Impl\r\n  {\r\n\r\n"), *skookum_class_name);

  // Export all functions
  for (TFieldIterator<UFunction> FuncIt(class_p /*, EFieldIteratorFlags::ExcludeSuper */); FuncIt; ++FuncIt)
    {
    UFunction * function_p = *FuncIt;
    if (can_export_method(class_p, function_p))
      {
      binding.make_method(function_p);
      if (bindings[Scope_instance].Find(binding) < 0 && bindings[Scope_class].Find(binding) < 0) // If method with this name already bound, assume it does the same thing and skip
        {
        generated_code += generate_method(class_name_cpp, class_p, function_p, binding);
        bindings[function_p->HasAnyFunctionFlags(FUNC_Static) ? Scope_class : Scope_instance].Push(binding);
        }
      }
    }

  // Export properties that are owned by this class
  int32 property_index = 0;
  for (TFieldIterator<UProperty> property_it(class_p /*, EFieldIteratorFlags::ExcludeSuper */); property_it; ++property_it, ++property_index)
    {
    UProperty * property_p = *property_it;
    if (can_export_property(class_p, property_p))
      {
      UE_LOG(LogSkookumScriptGenerator, Log, TEXT("  %s %s"), *property_p->GetClass()->GetName(), *property_p->GetName());

      binding.make_property_getter(property_p);
      if (bindings[Scope_instance].Find(binding) < 0)
        { // Only generate property accessor if not covered by a method already
        generated_code += generate_property_getter(class_name_cpp, class_p, property_p, property_index, binding);
        bindings[Scope_instance].Push(binding);
        }

      if (property_p->PropertyFlags & CPF_Edit)
        {
        binding.make_property_setter(property_p);
        if (bindings[Scope_instance].Find(binding) < 0)
          { // Only generate property accessor if not covered by a method already
          generated_code += generate_property_setter(class_name_cpp, class_p, property_p, property_index, binding);
          bindings[Scope_instance].Push(binding);
          }
        }
      }
    }

  // Binding array
  for (uint32 scope = 0; scope < 2; ++scope)
    {
    if (bindings[scope].Num() > 0)
      {
      generated_code += FString::Printf(TEXT("  static const SkClass::MethodInitializerFuncId methods_%c[] =\r\n    {\r\n"), scope ? TCHAR('c') : TCHAR('i'));
      for (auto & binding : bindings[scope])
        {
        generated_code += FString::Printf(TEXT("      { 0x%08x, mthd%s_%s },\r\n"), get_skookum_string_crc(*binding.m_script_name), scope ? TEXT("c") : TEXT(""), *binding.m_code_name);
        }
      generated_code += TEXT("    };\r\n\r\n");
      }
    }

  // Close namespace
  generated_code += FString::Printf(TEXT("  } // SkUE%s_Impl\r\n\r\n"), *skookum_class_name);

  // Register bindings function
  generated_code += FString::Printf(TEXT("void SkUE%s::register_bindings()\r\n  {\r\n"), *skookum_class_name);

  generated_code += FString::Printf(TEXT("  tBindingBase::register_bindings(0x%08x); // \"%s\"\r\n\r\n"), get_skookum_string_crc(*skookum_class_name), *skookum_class_name);

  // Only set ms_uclass_p if DLL exported
  if (does_class_have_static_class(class_p))
    {
    generated_code += FString::Printf(TEXT("  ms_uclass_p = %s::StaticClass();\r\n"), *class_name_cpp);
    }
  else
    {
    generated_code += FString::Printf(TEXT("  ms_uclass_p = FindObject<UClass>(ANY_PACKAGE, TEXT(\"%s\"));\r\n"), *class_p->GetName());
    }

  for (uint32 scope = 0; scope < 2; ++scope)
    {
    if (bindings[scope].Num() > 0)
      {
      generated_code += FString::Printf(TEXT("  ms_class_p->register_method_func_bulk(SkUE%s_Impl::methods_%c, %d, %s);\r\n"), *skookum_class_name, scope ? TCHAR('c') : TCHAR('i'), bindings[scope].Num(), scope ? TEXT("SkBindFlag_class_no_rebind") : TEXT("SkBindFlag_instance_no_rebind"));
      }
    }
  generated_code += TEXT("  }\r\n");

  save_header_if_changed(m_binding_code_path / class_binding_file_name, generated_code);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_struct_from_property(UProperty * prop_p)
  {
  UStruct * struct_p = Cast<UStructProperty>(prop_p)->Struct;
  if (!struct_p)
    return;

  generate_struct(struct_p, m_source_header_file_name);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_struct(UStruct * struct_p, const FString & source_header_file_name)
  {
  if (can_export_struct(struct_p))
    {
    UE_LOG(LogSkookumScriptGenerator, Log, TEXT("Generating struct %s"), *struct_p->GetName());

    m_exported_classes.Add(struct_p);

    // Generate script files
    generate_struct_script_files(struct_p);

    // Generate binding code files
    generate_struct_header_file(struct_p, source_header_file_name);
    generate_struct_binding_file(struct_p);
    }

    UStruct * parent_struct_p = struct_p->GetSuperStruct();
    if (parent_struct_p)
      generate_struct(parent_struct_p, source_header_file_name);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_struct_script_files(UStruct * struct_p)
  {
  const FString skookum_class_name = skookify_class_name(struct_p->GetName());
  const FString struct_class_path = get_skookum_class_path(struct_p);

  // Create class meta file:
  const FString meta_file_path = struct_class_path / TEXT("!Class.sk-meta");
  FString body = get_comment_block(struct_p);
  if (!FFileHelper::SaveStringToFile(body, *meta_file_path, ms_script_file_encoding))
    FError::Throwf(TEXT("Could not save file: %s"), *meta_file_path);

  // Constructor
  const FString ctor_file_path = struct_class_path / TEXT("!().sk");
  body = FString::Printf(TEXT("() %s\r\n"), *skookum_class_name);
  if (!FFileHelper::SaveStringToFile(body, *ctor_file_path, ms_script_file_encoding))
    FError::Throwf(TEXT("Could not save file: %s"), *ctor_file_path);

  // Copy constructor
  const FString ctor_copy_file_path = struct_class_path / TEXT("!copy().sk");
  body = FString::Printf(TEXT("(%s other) %s\r\n"), *skookum_class_name, *skookum_class_name);
  if (!FFileHelper::SaveStringToFile(body, *ctor_copy_file_path, ms_script_file_encoding))
    FError::Throwf(TEXT("Could not save file: %s"), *ctor_copy_file_path);

  // Assignment operator
  if (UScriptStruct * script_struct = Cast<UScriptStruct>(struct_p))
    {
    if (!!(script_struct->StructFlags & STRUCT_CopyNative))
      {
      const FString assign_file_path = struct_class_path / TEXT("assign().sk");
      body = FString::Printf(TEXT("(%s other) %s\r\n"), *skookum_class_name, *skookum_class_name);
      if (!FFileHelper::SaveStringToFile(body, *assign_file_path, ms_script_file_encoding))
        FError::Throwf(TEXT("Could not save file: %s"), *assign_file_path);
      }
    }

  // Destructor
  const FString d_torr_file_path = struct_class_path / TEXT("!!().sk");
  body = TEXT("()\r\n");
  if (!FFileHelper::SaveStringToFile(body, *d_torr_file_path, ms_script_file_encoding))
    FError::Throwf(TEXT("Could not save file: %s"), *d_torr_file_path);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_struct_header_file(UStruct * struct_p, const FString & source_header_file_name)
  {
  const FString class_header_file_name = FString::Printf(TEXT("SkUE%s.generated.hpp"), *get_skookum_class_name(struct_p));
  m_all_header_file_names.Add(class_header_file_name);

  FString skookum_class_name = get_skookum_class_name(struct_p);
  FString cpp_class_name = get_cpp_class_name(struct_p);

  FString generated_code;

  generated_code += TEXT("#pragma once\r\n\r\n");
  generated_code += TEXT("#include <Bindings/SkUEClassBinding.hpp>\r\n");

  // if not defined let's hope it is already known when the compiler gets here
  if (source_header_file_name.Len() > 0)
    {
    FString relative_path(source_header_file_name);
    FPaths::MakePathRelativeTo(relative_path, *m_runtime_plugin_root_path);
    generated_code += FString::Printf(TEXT("#include <%s>\r\n\r\n"), *relative_path);
    }
  else
    {
    generated_code += FString::Printf(TEXT("// Note: Include path for %s was unknown at code generation time, so hopefully the class is already known when compilation gets here\r\n\r\n"), *cpp_class_name);
    }

  generated_code += FString::Printf(TEXT("class SkUE%s : public SkClassBindingStruct<SkUE%s, %s>\r\n  {\r\n"),
    *skookum_class_name,
    *skookum_class_name,
    *cpp_class_name);

  generated_code += TEXT("  public:\r\n");

  if (UScriptStruct * script_struct = Cast<UScriptStruct>(struct_p))
    {
    generated_code += FString::Printf(TEXT("    enum { Binding_has_assign = %s };\r\n"), (script_struct->StructFlags & STRUCT_CopyNative) ? TEXT("true") : TEXT("false"));

    if (script_struct->StructFlags | STRUCT_CopyNative) // no copy assignment operator, override here to avoid compile error
      generated_code += TEXT("    static void mthd_op_assign(SkInvokedMethod * scope_p, SkInstance ** result_pp) {}\r\n");
    }

  generated_code += TEXT("    static void register_bindings();\r\n");
  generated_code += TEXT("  };\r\n");

  save_header_if_changed(m_binding_code_path / class_header_file_name, generated_code);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_struct_binding_file(UStruct * struct_p)
  {
  const FString skookum_class_name = skookify_class_name(struct_p->GetName());
  const FString struct_binding_file_name = FString::Printf(TEXT("SkUE%s.generated.inl"), *skookum_class_name);
  m_all_binding_file_names.Add(struct_binding_file_name);

  const FString class_name_cpp = get_cpp_class_name(struct_p);

  enum eScope { Scope_instance, Scope_class }; // 0 = instance, 1 = static bindings
  TArray<MethodBinding> bindings[2]; // eScope
  MethodBinding binding;

  FString generated_code;
  generated_code += FString::Printf(TEXT("\r\nnamespace SkUE%s_Impl\r\n  {\r\n\r\n"), *skookum_class_name);

  // Export properties that are owned by this struct
  int32 property_index = 0;
  for (TFieldIterator<UProperty> property_it(struct_p /*, EFieldIteratorFlags::ExcludeSuper*/); property_it; ++property_it, ++property_index)
    {
    UProperty * property_p = *property_it;
    if (can_export_property(struct_p, property_p))
      {
      UE_LOG(LogSkookumScriptGenerator, Log, TEXT("  %s %s"), *property_p->GetClass()->GetName(), *property_p->GetName());

      binding.make_property_getter(property_p);
      if (bindings[Scope_instance].Find(binding) < 0)
        { // Only generate property accessor if not covered by a method already
        generated_code += generate_property_getter(class_name_cpp, struct_p, property_p, property_index, binding);
        bindings[Scope_instance].Push(binding);
        }

      if (property_p->PropertyFlags & CPF_Edit)
        {
        binding.make_property_setter(property_p);
        if (bindings[Scope_instance].Find(binding) < 0)
          { // Only generate property accessor if not covered by a method already
          generated_code += generate_property_setter(class_name_cpp, struct_p, property_p, property_index, binding);
          bindings[Scope_instance].Push(binding);
          }
        }

      } // can_export_property
    } //for

    // Binding array
    for (uint32 scope = 0; scope < 2; ++scope)
      {
      if (bindings[scope].Num() > 0)
        {
        generated_code += FString::Printf(TEXT("  static const SkClass::MethodInitializerFuncId methods_%c[] =\r\n    {\r\n"), scope ? TCHAR('c') : TCHAR('i'));
        for (auto & binding : bindings[scope])
          {
          generated_code += FString::Printf(TEXT("      { 0x%08x, mthd%s_%s },\r\n"), get_skookum_string_crc(*binding.m_script_name), scope ? TEXT("c") : TEXT(""), *binding.m_code_name);
          }
          generated_code += TEXT("    };\r\n\r\n");
        }
      }

    // Close namespace
    generated_code += FString::Printf(TEXT("  } // SkUE%s_Impl\r\n\r\n"), *skookum_class_name);

    // Register bindings function
    generated_code += FString::Printf(TEXT("void SkUE%s::register_bindings()\r\n  {\r\n"), *skookum_class_name);
    generated_code += FString::Printf(TEXT("  tBindingBase::register_bindings(0x%08x); // \"%s\"\r\n\r\n"), get_skookum_string_crc(*skookum_class_name), *skookum_class_name);

    generated_code += FString::Printf(TEXT("  ms_ustruct_p = FindObject<UStruct>(ANY_PACKAGE, TEXT(\"%s\"));\r\n"), *struct_p->GetName());

    for (uint32 scope = 0; scope < 2; ++scope)
      {
      if (bindings[scope].Num() > 0)
        {
        generated_code += FString::Printf(TEXT("  ms_class_p->register_method_func_bulk(SkUE%s_Impl::methods_%c, %d, %s);\r\n"), *skookum_class_name, scope ? TCHAR('c') : TCHAR('i'), bindings[scope].Num(), scope ? TEXT("SkBindFlag_class_no_rebind") : TEXT("SkBindFlag_instance_no_rebind"));
        }
      }
    generated_code += TEXT("  }\r\n");

    save_header_if_changed(m_binding_code_path / struct_binding_file_name, generated_code);
  }

//---------------------------------------------------------------------------------------
void FSkookumScriptGenerator::generate_enum_from_property(UProperty * prop_p)
  {
  UEnum* enum_p = Cast<UByteProperty>(prop_p)->Enum;

  if (!enum_p)
    return;

  if (m_exported_enums.Contains(enum_p))
    return;

  m_exported_enums.Add(enum_p);

  generate_enum_script_files(enum_p);
  }

//---------------------------------------------------------------------------------------
void FSkookumScriptGenerator::generate_enum_script_files(UEnum * enum_p)
  {
  FString enum_type_name = enum_p->GetName();
  FString enum_path = m_scripts_path / TEXT("Object/Enum") / enum_type_name;

  // meta
  FString meta_file_path = enum_path / TEXT("!Class.sk-meta");
  FString meta_body = get_comment_block(enum_p).Replace(TEXT("this field"), TEXT("this enum"));
  if (!FFileHelper::SaveStringToFile(meta_body, *meta_file_path, ms_script_file_encoding))
    {
    FError::Throwf(TEXT("Could not save file: %s"), *meta_file_path);
    }

  // class data members and class constructor
  FString data_file_path = enum_path / TEXT("!DataC.sk");
  FString data_body;

  FString consructor_file_path = enum_path / TEXT("!()C.sk");
  FString enum_script_path = enum_p->GetPathName();
  FString consructor_body;

  for (int32 EnumIndex = 0; EnumIndex < enum_p->NumEnums() - 1; ++EnumIndex)
    {
    FString enum_val_name = enum_p->GetEnumName(EnumIndex);
    FString enum_val_full_name = enum_p->GenerateFullEnumName(*enum_val_name);

    FString skookified_val_nmae = skookify_var_name(enum_val_name);
    if (skookified_val_nmae.Equals(TEXT("world")) || skookified_val_nmae.Equals(TEXT("random")))
      {
      skookified_val_nmae += "_";
      }

    FName token = FName(*enum_val_full_name, FNAME_Find);
    if (token != NAME_None)
      {
      int32 Index = UEnum::LookupEnumName(token);
      if (Index != INDEX_NONE)
        {
        data_body += FString::Printf(TEXT("%s !@@%s\r\n"), *enum_type_name, *skookified_val_nmae);
        consructor_body += FString::Printf(TEXT("  @@%s: %s!int(%d)\r\n"), *skookified_val_nmae, *enum_type_name, Index);
        }
      }
    }

  if (!FFileHelper::SaveStringToFile(data_body, *data_file_path, ms_script_file_encoding))
    {
    FError::Throwf(TEXT("Could not save file: %s"), *data_file_path);
    }

  FString file_body = FString::Printf(TEXT("// %s\r\n// EnumPath: %s\r\n\r\n()\r\n\r\n  [\r\n%s  ]\r\n"), *enum_type_name, *enum_script_path, *consructor_body);
  if (!FFileHelper::SaveStringToFile(file_body, *consructor_file_path, ms_script_file_encoding))
    {
    FError::Throwf(TEXT("Could not save file: %s"), *consructor_file_path);
    }

  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_method(const FString & class_name_cpp, UClass * class_p, UFunction * function_p, const MethodBinding & binding)
  {
  // Generate script file
  generate_method_script_file(function_p, binding.m_script_name);

  // Generate binding code
  return generate_method_binding_code(class_name_cpp, class_p, function_p, binding);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_method_script_file(UFunction * function_p, const FString & script_function_name)
  {
  // Generate function content
  FString function_body = get_comment_block(function_p);
  bool has_params_or_return_value = (function_p->Children != NULL);
  if (has_params_or_return_value)
    {
    function_body += TEXT("(");

    FString separator;
    FString return_type_name;
    for (TFieldIterator<UProperty> param_it(function_p); param_it; ++param_it)
      {
      UProperty * param_p = *param_it;
      if (param_p->GetPropertyFlags() & CPF_ReturnParm)
        {
        return_type_name = get_skookum_property_type_name(param_p);
        }
      else
        {
        function_body += separator + get_skookum_property_type_name(param_p) + TEXT(" ") + skookify_var_name(param_p->GetName()) + get_skookum_default_initializer(function_p, param_p);
        }
      separator = TEXT(", ");
      }

    function_body += TEXT(") ") + return_type_name + TEXT("\n");
    }
  else
    {
    function_body = TEXT("()\n");
    }

  // Create script file
  FString function_file_path = get_skookum_method_path(function_p->GetOwnerClass(), script_function_name, function_p->HasAnyFunctionFlags(FUNC_Static));
  if (!FFileHelper::SaveStringToFile(function_body, *function_file_path, ms_script_file_encoding))
    {
    FError::Throwf(TEXT("Could not save file: %s"), *function_file_path);
    }
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_method_binding_code(const FString & class_name_cpp, UClass * class_p, UFunction * function_p, const MethodBinding & binding)
  {
  UProperty * return_value_p = NULL;

  bool is_static = function_p->HasAnyFunctionFlags(FUNC_Static);

  FString function_body;
  function_body += FString::Printf(TEXT("    %s\r\n"), *generate_this_pointer_initialization(class_name_cpp, class_p, is_static));

  FString params;
  FString out_params;

  const bool bHasParamsOrReturnValue = (function_p->Children != NULL);
  if (bHasParamsOrReturnValue)
    {
    params += TEXT("    struct FDispatchParams\r\n      {\r\n");

    for (TFieldIterator<UProperty> param_it(function_p); param_it; ++param_it)
      {
      UProperty * param = *param_it;
      params += FString::Printf(TEXT("      %s %s;\r\n"), *get_cpp_property_type_name(param, CPPF_ArgumentOrReturnValue), *param->GetName());
      }
    params += TEXT("      } params;\r\n");
    int32 ParamIndex = 0;
    for (TFieldIterator<UProperty> param_it(function_p); param_it; ++param_it, ++ParamIndex)
      {
      UProperty * param_p = *param_it;
      params += generate_method_parameter_expression(function_p, param_p, ParamIndex, FString::Printf(TEXT("    params.%s = %%s;\r\n"), /*format_string=*/ *param_p->GetName()));

      if ((param_p->GetPropertyFlags() & CPF_OutParm) && !(param_p->GetPropertyFlags() & CPF_ReturnParm))
        {
        FString param_in_struct = FString::Printf(TEXT("params.%s"), *param_p->GetName());
        out_params += FString::Printf(TEXT("%s;\r\n"), *generate_method_out_parameter_expression(function_p, param_p, ParamIndex, param_in_struct));
        }
      }
    }

  params += FString::Printf(TEXT("    SK_ASSERTX(this_p, \"Tried to invoke method %s@%s but the %s is null.\");\r\n"), *get_skookum_class_name(class_p), *binding.m_script_name, *get_skookum_class_name(class_p));
  params += TEXT("    if (this_p)\r\n      {\r\n");
  params += FString::Printf(TEXT("      static UFunction * function_p = this_p->FindFunctionChecked(TEXT(\"%s\"));\r\n"), *function_p->GetName());

  if (bHasParamsOrReturnValue)
    {
    params += TEXT("      check(function_p->ParmsSize <= sizeof(FDispatchParams));\r\n");
    params += TEXT("      this_p->ProcessEvent(function_p, &params);\r\n");
    }
  else
    {
    params += TEXT("      this_p->ProcessEvent(function_p, nullptr);\r\n");
    }

  params += TEXT("      }\r\n");

  function_body += params;
  function_body += out_params;

  FString function_call_argumants;
  FString return_value_declaration;
  for (TFieldIterator<UProperty> param_it(function_p); !return_value_p && param_it; ++param_it)
    {
    UProperty * param_p = *param_it;
    if (param_p->GetPropertyFlags() & CPF_ReturnParm)
      {
      return_value_p = param_p;
      }
    }
  if (return_value_p)
    {
    FString return_value_name = FString::Printf(TEXT("params.%s"), *return_value_p->GetName());
    function_body += generate_return_or_param_value_passing(class_p, function_p, return_value_p, *return_value_name, /*result_format_string=*/ TEXT("    if (result_pp) *result_pp = %s;\r\n"));
    }

  FString generated_code = FString::Printf(TEXT("  %s\r\n    {\r\n"), *generate_method_binding_declaration(*binding.m_code_name, is_static));
  generated_code += function_body;
  generated_code += TEXT("    }\r\n\r\n");

  return generated_code;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_property_getter(const FString & class_name_cpp, UStruct * class_or_struct_p, UProperty * property_p, int32 property_index, const MethodBinding & binding)
  {
  // Generate script file
  generate_property_getter_script_file(property_p, binding.m_script_name);

  // Generate binding code
  return generate_property_getter_binding_code(property_p, class_name_cpp, class_or_struct_p, binding);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_property_getter_script_file(UProperty * property_p, const FString & script_function_name)
  {
  FString body = get_comment_block(property_p);
  body += TEXT("() ") + get_skookum_property_type_name(property_p) + TEXT("\n");
  UStruct * owner = property_p->GetOwnerClass() ? property_p->GetOwnerClass() : property_p->GetOwnerStruct();
  FString function_file_path = get_skookum_method_path(owner, script_function_name, false);
  if (!FFileHelper::SaveStringToFile(body, *function_file_path, ms_script_file_encoding))
    {
    FError::Throwf(TEXT("Could not save file: %s"), *function_file_path);
    }
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_property_getter_binding_code(UProperty * property_p, const FString & class_name_cpp, UStruct * class_or_struct_p, const MethodBinding & binding)
  {
  bool is_class = Cast<UClass>(class_or_struct_p) != nullptr;

  FString generated_code;

  FString default_ctor_argument = generate_property_default_ctor_argument(property_p);
  if (!default_ctor_argument.IsEmpty())
    {
    default_ctor_argument = TEXT("(") + default_ctor_argument + TEXT(")");
    }

  generated_code += FString::Printf(TEXT("  %s\r\n    {\r\n"), *generate_method_binding_declaration(binding.m_code_name, false));

  if (is_class)
    {
    generated_code += FString::Printf(TEXT("    %s\r\n"), *generate_this_pointer_initialization(class_name_cpp, class_or_struct_p, false));
    generated_code += FString::Printf(TEXT("    static UProperty * property_p = SkUEClassBindingHelper::find_class_property(SkUE%s::ms_uclass_p, TEXT(\"%s\"));\r\n"), *get_skookum_class_name(class_or_struct_p), *property_p->GetName());
    }
  else
    {
    generated_code += FString::Printf(TEXT("    %s * this_p = &(scope_p->this_as<SkUE%s>());\r\n"), *class_name_cpp, *get_skookum_class_name(class_or_struct_p));
    generated_code += FString::Printf(TEXT("    static UProperty * property_p = SkUE%s::ms_ustruct_p->FindPropertyByName(TEXT(\"%s\"));\r\n"), *get_skookum_class_name(class_or_struct_p), *property_p->GetName());
    }

  generated_code += FString::Printf(TEXT("    %s property_value%s;\r\n"), *get_cpp_property_type_name(property_p, CPPF_ArgumentOrReturnValue), *default_ctor_argument);
  generated_code += FString::Printf(TEXT("    SK_ASSERTX(this_p, \"Tried to invoke method %s@%s but the %s is null.\");\r\n"), *get_skookum_class_name(class_or_struct_p), *binding.m_script_name, *get_skookum_class_name(class_or_struct_p));
  generated_code += TEXT("    if (this_p)\r\n      {\r\n");
  generated_code += TEXT("      property_p->CopyCompleteValue(&property_value, property_p->ContainerPtrToValuePtr<void>(this_p));\r\n");
  generated_code += TEXT("      }\r\n");
  generated_code += generate_return_or_param_value_passing(class_or_struct_p, NULL, property_p, TEXT("property_value"), /*result_format_string=*/ TEXT("    if (result_pp) *result_pp = %s;\r\n"));
  generated_code += TEXT("    }\r\n\r\n");

  return generated_code;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_property_setter(const FString & class_name_cpp, UStruct * class_or_struct_p, UProperty * property_p, int32 property_index, const MethodBinding & binding)
  {
  // Generate script files
  generate_property_setter_script_file(property_p, binding.m_script_name);

  // Generate binding code
  return generate_property_setter_binding_code(property_p, class_name_cpp, class_or_struct_p, binding);
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_property_setter_script_file(UProperty * property_p, const FString & script_function_name)
  {
  FString var_name = skookify_var_name(property_p->GetName());
  FString file_name = script_function_name + TEXT("().sk");
  UStruct * owner = property_p->GetOwnerClass() ? property_p->GetOwnerClass() : property_p->GetOwnerStruct();
  FString class_path = get_skookum_class_path(owner);
  FString body = get_comment_block(property_p);
  body += TEXT("(") + get_skookum_property_type_name(property_p) + TEXT(" ") + var_name + TEXT(")\n");
  FString path = class_path / file_name;
  if (!FFileHelper::SaveStringToFile(body, *path, ms_script_file_encoding))
    {
    FError::Throwf(TEXT("Could not save file: %s"), *path);
    }
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_property_setter_binding_code(UProperty * property_p, const FString & class_name_cpp, UStruct * class_or_struct_p, const MethodBinding & binding)
  {
  bool is_class = Cast<UClass>(class_or_struct_p) != nullptr;

  FString generated_code;

  generated_code += FString::Printf(TEXT("  %s\r\n    {\r\n"), *generate_method_binding_declaration(binding.m_code_name, false));

  if (is_class)
    generated_code += FString::Printf(TEXT("    %s\r\n"), *generate_this_pointer_initialization(class_name_cpp, class_or_struct_p, false));
  else
    generated_code += FString::Printf(TEXT("    %s * this_p = &(scope_p->this_as<SkUE%s>());\r\n"), *class_name_cpp, *get_skookum_class_name(class_or_struct_p));

  generated_code += FString::Printf(TEXT("    SK_ASSERTX(this_p, \"Tried to invoke method %s@%s but the %s is null.\");\r\n"), *get_skookum_class_name(class_or_struct_p), *binding.m_script_name, *get_skookum_class_name(class_or_struct_p));
  generated_code += TEXT("    if (this_p)\r\n      {\r\n");

  if (is_class)
    generated_code += FString::Printf(TEXT("      static UProperty * property_p = SkUEClassBindingHelper::find_class_property(SkUE%s::ms_uclass_p, TEXT(\"%s\"));\r\n"), *get_skookum_class_name(class_or_struct_p), *property_p->GetName());
  else
    generated_code += FString::Printf(TEXT("      static UProperty * property_p = SkUE%s::ms_ustruct_p->FindPropertyByName(TEXT(\"%s\"));\r\n"), *get_skookum_class_name(class_or_struct_p), *property_p->GetName());

  generated_code += generate_method_parameter_expression(NULL, property_p, 0, /* format_string= */ FString::Printf(TEXT("      %s property_value = %%s;\r\n"), *get_cpp_property_type_name(property_p, CPPF_ArgumentOrReturnValue)));
  generated_code += TEXT("      property_p->CopyCompleteValue(property_p->ContainerPtrToValuePtr<void>(this_p), &property_value);\r\n");
  generated_code += TEXT("      }\r\n");

  generated_code += TEXT("    if (result_pp)\r\n");
  generated_code += TEXT("      {\r\n");
  generated_code += TEXT("      SkInstance * instance = scope_p->get_this(); \r\n");
  generated_code += TEXT("      instance->reference(); \r\n");
  generated_code += TEXT("      *result_pp = instance; \r\n");
  generated_code += TEXT("      }\r\n");
  generated_code += TEXT("    }\r\n\r\n");

  return generated_code;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_method_binding_declaration(const FString & function_name, bool is_static)
  {
  return FString::Printf(TEXT("static void mthd%s_%s(SkInvokedMethod * scope_p, SkInstance ** result_pp)"), is_static ? TEXT("c") : TEXT(""), *function_name);
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_this_pointer_initialization(const FString & class_name_cpp, UStruct * class_or_struct_p, bool is_static)
  {
  FString class_name_skookum = get_skookum_class_name(class_or_struct_p);
  if (is_static)
    {
    return FString::Printf(TEXT("%s * this_p = GetMutableDefault<%s>(SkUE%s::ms_uclass_p);"), *class_name_cpp, *class_name_cpp, *class_name_skookum);
    }
  else
    {
    return FString::Printf(TEXT("%s * this_p = scope_p->this_as<SkUE%s>();"), *class_name_cpp, *class_name_skookum);
    }
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_method_out_parameter_expression(UFunction * function_p, UProperty * param_p, int32 ParamIndex, const FString & param_name)
  {
  FString Initializer;

  eSkTypeID type_id = get_skookum_property_type(param_p);
  switch (type_id)
    {
    case SkTypeID_Integer:         Initializer = TEXT("    scope_p->get_arg<SkInteger>(SkArg_%d) = %s"); break;
    case SkTypeID_Real:            Initializer = TEXT("    scope_p->get_arg<SkReal>(SkArg_%d) = %s"); break;
    case SkTypeID_Boolean:         Initializer = TEXT("    scope_p->get_arg<SkBoolean>(SkArg_%d) = %s"); break;
    case SkTypeID_String:          Initializer = TEXT("    scope_p->get_arg<SkString>(SkArg_%d) = AString(*%s, %s.Len())"); break; // $revisit MBreyer - Avoid copy here
    case SkTypeID_Name:            Initializer = TEXT("    scope_p->get_arg<SkUEName>(SkArg_%d) = %s"); break;
    case SkTypeID_Vector2:         Initializer = TEXT("    scope_p->get_arg<SkVector2>(SkArg_%d) = %s"); break;
    case SkTypeID_Vector3:         Initializer = TEXT("    scope_p->get_arg<SkVector3>(SkArg_%d) = %s"); break;
    case SkTypeID_Vector4:         Initializer = TEXT("    scope_p->get_arg<SkVector4>(SkArg_%d) = %s"); break;
    case SkTypeID_Rotation:        Initializer = TEXT("    scope_p->get_arg<SkRotation>(SkArg_%d) = %s"); break;
    case SkTypeID_RotationAngles:  Initializer = TEXT("    scope_p->get_arg<SkRotationAngles>(SkArg_%d) = %s"); break;
    case SkTypeID_Transform:       Initializer = TEXT("    scope_p->get_arg<SkTransform>(SkArg_%d) = %s"); break;
    case SkTypeID_Color:           Initializer = TEXT("    scope_p->get_arg<SkColor>(SkArg_%d) = %s"); break;
    case SkTypeID_Enum:            Initializer = TEXT("    scope_p->get_arg<SkEnum>(SkArg_%d) = %s"); break;
    case SkTypeID_UStruct:         Initializer = FString::Printf(TEXT("    scope_p->get_arg<SkUE%s>(SkArg_%%d) = %%s"), *get_skookum_property_type_name(param_p)); break;
    case SkTypeID_UClass:          Initializer = TEXT("    scope_p->get_arg<SkUEEntityClass>(SkArg_%d) = %s"); break;
    case SkTypeID_UObject:         Initializer = FString::Printf(TEXT("    scope_p->get_arg<SkUE%s>(SkArg_%%d) = %%s"), *get_skookum_property_type_name(param_p)); break;
    case SkTypeID_List:            Initializer = generate_return_or_param_value_passing(nullptr, nullptr, param_p, *param_name,
       /*result_format_string=*/ FString::Printf(TEXT("    scope_p->get_arg<SkList>(SkArg_%d) = %%s->as<SkList>()"), ParamIndex + 1), ParamIndex); break;
    default:                      FError::Throwf(TEXT("Unsupported function param type: %s"), *param_p->GetClass()->GetName()); break;
    }

    if (type_id == SkTypeID_List)
      return Initializer;

    return FString::Printf(*Initializer, ParamIndex + 1, *param_name, *param_name);
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_method_parameter_expression(UFunction * function_p, UProperty * param_p, int32 ParamIndex, FString format_string)
  {
  // We assume a parameter goes out only if it is either the return value (of course)
  // or if it is marked CPF_OutParm _and_ its name begins with "Out"
  bool is_out_only = (param_p->GetPropertyFlags() & CPF_ReturnParm)
    || ((param_p->GetPropertyFlags() & CPF_OutParm) && param_p->GetName().Find(TEXT("Out")) == 0);
  // If it's not a purely outgoing parameter, fetch it from the caller
  if (!is_out_only)
    {
    FString Initializer;

    eSkTypeID type_id = get_skookum_property_type(param_p);
    switch (type_id)
      {
      case SkTypeID_Integer:         Initializer = TEXT("int(scope_p->get_arg<SkInteger>(SkArg_%d))"); break;
      case SkTypeID_Real:            Initializer = TEXT("float(scope_p->get_arg<SkReal>(SkArg_%d))"); break;
      case SkTypeID_Boolean:         Initializer = TEXT("scope_p->get_arg<SkBoolean>(SkArg_%d)"); break;
      case SkTypeID_String:          Initializer = TEXT("FString(scope_p->get_arg<SkString>(SkArg_%d).as_cstr())"); break; // $revisit MBreyer - Avoid copy here
      case SkTypeID_Name:            Initializer = TEXT("scope_p->get_arg<SkUEName>(SkArg_%d)"); break;
      case SkTypeID_Vector2:         Initializer = TEXT("scope_p->get_arg<SkVector2>(SkArg_%d)"); break;
      case SkTypeID_Vector3:         Initializer = TEXT("scope_p->get_arg<SkVector3>(SkArg_%d)"); break;
      case SkTypeID_Vector4:         Initializer = TEXT("scope_p->get_arg<SkVector4>(SkArg_%d)"); break;
      case SkTypeID_Rotation:        Initializer = TEXT("scope_p->get_arg<SkRotation>(SkArg_%d)"); break;
      case SkTypeID_RotationAngles:  Initializer = TEXT("scope_p->get_arg<SkRotationAngles>(SkArg_%d)"); break;
      case SkTypeID_Transform:       Initializer = TEXT("scope_p->get_arg<SkTransform>(SkArg_%d)"); break;
      case SkTypeID_Color:           Initializer = TEXT("scope_p->get_arg<SkColor>(SkArg_%d)"); break;
      case SkTypeID_Enum:            Initializer = FString::Printf(TEXT("(%s)( static_cast<uint8>(scope_p->get_arg<SkEnum>(SkArg_%%d)) )"), *get_cpp_property_type_name(param_p, CPPF_ArgumentOrReturnValue)); break;
      case SkTypeID_UStruct:         Initializer = FString::Printf(TEXT("scope_p->get_arg<SkUE%s>(SkArg_%%d)"), *get_skookum_property_type_name(param_p)); break;
      case SkTypeID_UClass:          Initializer = TEXT("scope_p->get_arg<SkUEEntityClass>(SkArg_%d)"); break;
      case SkTypeID_UObject:         Initializer = FString::Printf(TEXT("scope_p->get_arg<SkUE%s>(SkArg_%%d)"), *get_skookum_property_type_name(param_p)); break;
      case SkTypeID_List:            Initializer = get_array_parameter_expression(param_p, ParamIndex); break;
      default:                       FError::Throwf(TEXT("Unsupported function param type: %s"), *param_p->GetClass()->GetName()); break;
      }
    if (type_id == SkTypeID_List)
      {
      return Initializer + FString::Printf(*format_string, *FString::Printf(TEXT("param_arr_%d"), ParamIndex + 1));
      }
    return FString::Printf(*format_string, *FString::Printf(*Initializer, ParamIndex + 1));
    }
  else if (param_p->IsA(UObjectPropertyBase::StaticClass()) || param_p->IsA(UClassProperty::StaticClass()))
    {
    return FString::Printf(*format_string, TEXT("nullptr"));
    }
  else
    {
    return FString::Printf(*format_string, *FString::Printf(TEXT("%s(%s)"), *get_cpp_property_type_name(param_p, CPPF_ArgumentOrReturnValue), *generate_property_default_ctor_argument(param_p)));
    }
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_property_default_ctor_argument(UProperty * param_p)
  {
  eSkTypeID type_id = get_skookum_property_type(param_p);
  switch (type_id)
    {
    case SkTypeID_Integer:         return TEXT("0");
    case SkTypeID_Real:            return TEXT("0.0f");
    case SkTypeID_Boolean:         return TEXT("false");
    case SkTypeID_Enum:            return FString::Printf(TEXT("(%s)0"), *get_cpp_property_type_name(param_p, CPPF_ArgumentOrReturnValue));
    case SkTypeID_List:
    case SkTypeID_String:          
    case SkTypeID_Name:            
    case SkTypeID_Transform:
    case SkTypeID_UStruct:         return TEXT("");
    case SkTypeID_Vector2:         
    case SkTypeID_Vector3:         
    case SkTypeID_Vector4:         
    case SkTypeID_Rotation:        
    case SkTypeID_RotationAngles:  
    case SkTypeID_Color:           return TEXT("ForceInitToZero");
    case SkTypeID_UClass:          
    case SkTypeID_UObject:         return TEXT("nullptr");
    default:                       FError::Throwf(TEXT("Unsupported property type: %s"), *param_p->GetClass()->GetName()); return TEXT("");
    }
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::generate_return_or_param_value_passing(UStruct * class_or_struct_p, UFunction * function_p, UProperty * value_property, const FString & value_name, const FString & result_format_string, int32 ParamIndex /* =-1 */)
  {
  if (value_property)
    {
    FString fmt;

    eSkTypeID type_id = get_skookum_property_type(value_property);
    switch (type_id)
      {
      case SkTypeID_Integer:         fmt = TEXT("SkInteger::new_instance(%s)"); break;
      case SkTypeID_Real:            fmt = TEXT("SkReal::new_instance(%s)"); break;
      case SkTypeID_Boolean:         fmt = TEXT("SkBoolean::new_instance(%s)"); break;
      case SkTypeID_String:          fmt = TEXT("SkString::new_instance(AString(*(%s), %s.Len()))"); break; // $revisit MBreyer - Avoid copy here
      case SkTypeID_Name:            fmt = TEXT("SkUEName::new_instance(%s)"); break;
      case SkTypeID_Vector2:         fmt = TEXT("SkVector2::new_instance(%s)"); break;
      case SkTypeID_Vector3:         fmt = TEXT("SkVector3::new_instance(%s)"); break;
      case SkTypeID_Vector4:         fmt = TEXT("SkVector4::new_instance(%s)"); break;
      case SkTypeID_Rotation:        fmt = TEXT("SkRotation::new_instance(%s)"); break;
      case SkTypeID_RotationAngles:  fmt = TEXT("SkRotationAngles::new_instance(%s)"); break;
      case SkTypeID_Transform:       fmt = TEXT("SkTransform::new_instance(%s)"); break;
      case SkTypeID_Color:           fmt = TEXT("SkColor::new_instance(%s)"); break;
      case SkTypeID_Enum:            fmt = TEXT("SkEnum::new_instance((SkEnumType)%s,SkBrain::get_class(\"Enum\"))"); break;
      case SkTypeID_UStruct:         fmt = FString::Printf(TEXT("SkUE%s::new_instance(%%s)"), *get_skookum_property_type_name(value_property)); break;
      case SkTypeID_UClass:          fmt = TEXT("SkUEEntityClass::new_instance(%s)"); break;
      case SkTypeID_UObject:         fmt = FString::Printf(TEXT("SkUE%s::new_instance(%%s)"), *get_skookum_property_type_name(value_property)); break;
      case SkTypeID_List:
        {
        const UArrayProperty* ArrayProperty = Cast<UArrayProperty>(value_property);
        UProperty* inner_property = ArrayProperty->Inner;

        fmt =  FString::Printf(TEXT("    %s out_arr_%d = %%s;\r\n"), *get_cpp_property_type_name(value_property, CPPF_ArgumentOrReturnValue), ParamIndex + 1);
        fmt += FString::Printf(TEXT("    SkInstance * out_instance_%d = SkList::new_instance(out_arr_%d.Num());\r\n"), ParamIndex + 1, ParamIndex + 1);
        fmt += FString::Printf(TEXT("    SkInstanceList & out_list_%d = out_instance_%d->as<SkList>();\r\n"), ParamIndex + 1, ParamIndex + 1);
        fmt += FString::Printf(TEXT("    APArray<SkInstance> & out_instances_%d = out_list_%d.get_instances();\r\n"), ParamIndex + 1, ParamIndex + 1);
        fmt += FString::Printf(TEXT("    int32_t out_len_%d = out_arr_%d.Num();\r\n"), ParamIndex + 1, ParamIndex + 1);
        fmt += FString::Printf(TEXT("    for (int32 i = 0; i<out_len_%d; ++i)\r\n"), ParamIndex + 1);
        fmt +=                 TEXT("      {\r\n");
        fmt += generate_return_or_param_value_passing(class_or_struct_p, function_p, inner_property, /*value_name=*/ FString::Printf(TEXT("out_arr_%d[i]"), ParamIndex + 1), 
                                                                                 /*result_format_string=*/ FString::Printf(TEXT("      out_instances_%d.append(*(%%s));\r\n"), ParamIndex + 1));
        fmt +=                 TEXT("      }\r\n");
        }
        break;
      default:  FError::Throwf(TEXT("Unsupported return or param type: %s"), *value_property->GetClass()->GetName()); break;
      }

    FString initializer = FString::Printf(*fmt, *value_name, *value_name);

    if (type_id == SkTypeID_List)
      return FString::Printf(*(initializer + result_format_string), /*value_string=*/ *FString::Printf(TEXT("out_instance_%d"), ParamIndex + 1));

    return FString::Printf(*result_format_string, /*value_string=*/ *initializer);
    }
  else
    {
    return TEXT(""); // TEXT("if (result_pp) *result_pp = SkBrain::ms_nil_p;");
    }
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::generate_master_binding_file()
  {
  FString generated_code;

  generated_code += TEXT("\r\n");

  generated_code += TEXT("#include \"SkookumScript/SkClass.hpp\"\r\n");
  generated_code += TEXT("#include \"SkookumScript/SkBrain.hpp\"\r\n");
  generated_code += TEXT("#include \"SkookumScript/SkInvokedMethod.hpp\"\r\n");
  generated_code += TEXT("#include \"SkookumScript/SkInteger.hpp\"\r\n");
  generated_code += TEXT("#include \"SkookumScript/SkReal.hpp\"\r\n");
  generated_code += TEXT("#include \"SkookumScript/SkBoolean.hpp\"\r\n");
  generated_code += TEXT("#include \"SkookumScript/SkString.hpp\"\r\n");

  generated_code += TEXT("\r\n");

  // Include all headers
  for (auto & header_file_name : m_all_header_file_names)
    {
    // Re-base to make sure we're including the right files on a remote machine
    FString NewFilename(FPaths::GetCleanFilename(header_file_name));
    generated_code += FString::Printf(TEXT("#include \"%s\"\r\n"), *NewFilename);
    }

  generated_code += TEXT("\r\n");

  // Include all bindings
  for (auto & binding_file_name : m_all_binding_file_names)
    {
    // Re-base to make sure we're including the right files on a remote machine
    FString NewFilename(FPaths::GetCleanFilename(binding_file_name));
    generated_code += FString::Printf(TEXT("#include \"%s\"\r\n"), *NewFilename);
    }

  generated_code += TEXT("\r\nnamespace SkUE\r\n  {\r\n\r\n");

  generated_code += TEXT("  void register_bindings()\r\n    {\r\n");
  for (auto class_p : m_exported_classes)
    {
    generated_code += FString::Printf(TEXT("    SkUE%s::register_bindings();\r\n"), *get_skookum_class_name(class_p));
    }
  generated_code += FString::Printf(TEXT("\r\n    SkUEClassBindingHelper::reset_static_class_mappings(%d);\r\n"), m_exported_classes.Num());
  for (auto class_p : m_exported_classes)
    {
    if (Cast<UClass>(class_p))
      generated_code += FString::Printf(TEXT("    SkUEClassBindingHelper::add_static_class_mapping(SkUE%s::ms_class_p, SkUE%s::ms_uclass_p);\r\n"), *get_skookum_class_name(class_p), *get_skookum_class_name(class_p));
    }
  generated_code += TEXT("\r\n    }\r\n");

  generated_code += TEXT("\r\n  } // SkUE\r\n");

  FString master_binding_file_name = m_binding_code_path / TEXT("SkUE.generated.inl");
  save_header_if_changed(master_binding_file_name, generated_code);
  }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::can_export_class(UClass * class_p, const FString & source_header_file_name) const
  {
  FString class_name = *class_p->GetName();

  return does_class_have_static_class(class_p) // Don't export classes that don't export DLL symbols
    && !m_exported_classes.Contains(class_p) // Don't export classes that have already been exported
    && !m_skip_classes.Contains(class_name); // Don't export classes that set to skip in UHT config file
  }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::can_export_method(UClass * class_p, UFunction * function_p)
  {
  // If this function is inherited, do nothing as SkookumScript will inherit it for us
  if (function_p->GetOwnerClass() != class_p)
    return false;

  // We don't support delegates and non-public functions
  if ((function_p->FunctionFlags & FUNC_Delegate))
    {
    return false;
    }

  // Reject if any of the parameter types is unsupported yet
  for (TFieldIterator<UProperty> param_it(function_p); param_it; ++param_it)
    {
    UProperty * param_p = *param_it;

    //if (param_p->IsA(UArrayProperty::StaticClass()) ||
    //  param_p->ArrayDim > 1 ||
    if (param_p->IsA(UDelegateProperty::StaticClass()) ||
      param_p->IsA(UMulticastDelegateProperty::StaticClass()) ||
      param_p->IsA(UWeakObjectProperty::StaticClass()) ||
      param_p->IsA(UInterfaceProperty::StaticClass()))
      {
      return false;
      }

    if (!is_property_type_supported(param_p))
      {
      return false;
      }

    eSkTypeID type_id= get_skookum_property_type(param_p);

    if (type_id == SkTypeID_Enum)
      generate_enum_from_property(param_p);

    if (type_id == SkTypeID_UStruct)
      generate_struct_from_property(param_p);

    }

  return true;
  }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::can_export_property(UStruct * class_or_struct_p, UProperty * property_p)
  {
  UClass * class_p = Cast<UClass>(class_or_struct_p);
  // If this property is inherited, do nothing as SkookumScript will inherit it for us
  if (class_p && property_p->GetOwnerClass() != class_p)
    return false;

  if (property_p->GetOwnerStruct() != class_or_struct_p)
    return false;

  // Check if property type is supported
  if (!is_property_type_supported(property_p))
    return false;

  eSkTypeID type_id = get_skookum_property_type(property_p);

  if (type_id == SkTypeID_Enum)
    generate_enum_from_property(property_p);

  if (type_id == SkTypeID_UStruct)
    generate_struct_from_property(property_p);

  return true;
  }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::can_export_struct(UStruct * struct_p)
  {
  if (m_exported_classes.Contains(struct_p) || m_skip_classes.Contains(struct_p->GetName()))
    return false;

  return is_struct_type_supported(struct_p);
  }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::is_struct_type_supported(UStruct * struct_p)
  {
  UScriptStruct * script_struct = Cast<UScriptStruct>(struct_p);
  if (script_struct && (script_struct->HasDefaults() || !!(script_struct->StructFlags & STRUCT_RequiredAPI)))
    return true;

  return false;
  }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::does_class_have_static_class(UClass * class_p)
  {
  return class_p->HasAnyClassFlags(CLASS_RequiredAPI | CLASS_MinimalAPI);
  }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::is_property_type_supported(UProperty * property_p)
  {
  if (property_p->IsA(ULazyObjectProperty::StaticClass())
    || property_p->IsA(UAssetObjectProperty::StaticClass())
    || property_p->IsA(UAssetClassProperty::StaticClass())
    || property_p->IsA(UWeakObjectProperty::StaticClass()))
    {
    return false;
    }

  return (get_skookum_property_type(property_p) != SkTypeID_None);
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::skookify_class_name(const FString & name)
  {
  if (name == TEXT("Object")) return TEXT("Entity");
  if (name == TEXT("Class"))  return TEXT("EntityClass");
  if (name == TEXT("Enum"))   return TEXT("Enum2"); // HACK

  return name;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::skookify_var_name(const FString & name, bool * is_boolean_p)
  {
  if (name.IsEmpty()) return name;

  // Change title case to lower case with underscores
  FString skookum_name;
  skookum_name.Reserve(name.Len() + 16);
  bool is_boolean = name[0] == 'b' && isupper(name[1]);
  bool was_upper = true;
  bool was_underscore = true;
  for (int32 i = int32(is_boolean); i < name.Len(); ++i)
    {
    TCHAR c = name[i];
    bool is_upper = isupper(c) != 0 || isdigit(c) != 0;
    if (is_upper && !was_upper && !was_underscore)
      {
      skookum_name.AppendChar('_');
      }
    skookum_name.AppendChar(tolower(c));
    was_upper = is_upper;
    was_underscore = (c == TCHAR('_'));
    }

  // Check for reserved keywords and append underscore if found
  for (uint32 i = 0; i < sizeof(ms_reserved_keywords) / sizeof(ms_reserved_keywords[0]); ++i)
    {
    if (skookum_name == ms_reserved_keywords[i])
      {
      skookum_name.AppendChar('_');
      break;
      }
    }

  if (is_boolean_p) *is_boolean_p = is_boolean;
  return skookum_name;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::skookify_method_name(const FString & name, UProperty * return_property_p)
  {
  bool is_boolean = false;
  FString method_name = skookify_var_name(name, &is_boolean);

  // Remove K2 (Kismet 2) prefix if present
  if (method_name.Len() > 3 && !method_name.Mid(3, 1).IsNumeric())
    {
    method_name.RemoveFromStart(TEXT("k2_"), ESearchCase::CaseSensitive);
    }

  if (method_name.Len() > 4 && !method_name.Mid(4, 1).IsNumeric())
    {
    // If name starts with "get_", remove it
    if (method_name.RemoveFromStart(TEXT("get_"), ESearchCase::CaseSensitive))
      {
      // Append question mark
      is_boolean = true;
      }
    // If name starts with "set_", remove it and append "_set" instead
    else if (method_name.RemoveFromStart(TEXT("set_"), ESearchCase::CaseSensitive))
      {
      method_name.Append(TEXT("_set"));
      }
    }

  // If name starts with "is_", "has_" or "can_" also append question mark
  if (method_name.Find(TEXT("is_"), ESearchCase::CaseSensitive) == 0
   || method_name.Find(TEXT("has_"), ESearchCase::CaseSensitive) == 0
   || method_name.Find(TEXT("can_"), ESearchCase::CaseSensitive) == 0)
    {
    is_boolean = true;
    }

  // Append question mark if determined to be boolean
  if (is_boolean && return_property_p && return_property_p->IsA(UBoolProperty::StaticClass()))
    {
    method_name += TEXT("?");
    }

  return method_name;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::get_skookum_class_name(UStruct * class_or_struct_p)
  {
  return skookify_class_name(class_or_struct_p->GetName());
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::get_skookum_class_path(UStruct * class_or_struct_p)
  {
  bool is_class = Cast<UClass>(class_or_struct_p) != nullptr;

  // Remember class name
  FString class_name = skookify_class_name(class_or_struct_p->GetName());

  // Make array of the super classes
  TArray<UStruct *> super_class_stack;
  super_class_stack.Reserve(32);
  UStruct * super_p = class_or_struct_p;

  while ((super_p = is_class ? Cast<UClass>(super_p)->GetSuperClass() : super_p->GetSuperStruct()) != nullptr)
    {
    super_class_stack.Push(super_p);
    if (is_class)
      m_used_classes.AddUnique(Cast<UClass>(super_p)); // all super classes are also considered used
    }

  // Build path
  int32 max_super_class_nesting = is_class ? FMath::Max(m_scripts_path_depth - 1, 0) : FMath::Max(m_scripts_path_depth - 2, 0);
  FString class_path = is_class ? m_scripts_path / TEXT("Object") : m_scripts_path / TEXT("Object/UStruct");
  for (int32 i = 0; i < max_super_class_nesting && super_class_stack.Num(); ++i)
    {
    class_path /= skookify_class_name(super_class_stack.Pop()->GetName());
    }
  if (super_class_stack.Num())
    {
    class_name = skookify_class_name(super_class_stack[0]->GetName()) + TEXT(".") + class_name;
    }
  return class_path / class_name;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::get_skookum_method_path(UStruct * class_or_struct_p, const FString & script_function_name, bool is_static)
  {
  return get_skookum_class_path(class_or_struct_p) / (script_function_name.Replace(TEXT("?"), TEXT("-Q")) + (is_static ? TEXT("()C.sk") : TEXT("().sk")));
  }

//---------------------------------------------------------------------------------------

FSkookumScriptGenerator::eSkTypeID FSkookumScriptGenerator::get_skookum_property_type(UProperty * property_p)
  {
  // Check for simple types first
  if (property_p->IsA(UIntProperty::StaticClass()))         return SkTypeID_Integer;
  if (property_p->IsA(UFloatProperty::StaticClass()))       return SkTypeID_Real;
  if (property_p->IsA(UStrProperty::StaticClass()))         return SkTypeID_String;
  if (property_p->IsA(UNameProperty::StaticClass()))        return SkTypeID_Name;
  if (property_p->IsA(UBoolProperty::StaticClass()))        return SkTypeID_Boolean;

  // Any known struct?
  if (property_p->IsA(UStructProperty::StaticClass()))
    {
    // Supported structs
    static FName name_Vector2D("Vector2D");
    static FName name_Vector("Vector");
    static FName name_Vector4("Vector4");
    static FName name_Quat("Quat");
    static FName name_Rotator("Rotator");
    static FName name_Transform("Transform");
    static FName name_LinearColor("LinearColor");
    static FName name_Color("Color");

    UStructProperty * struct_prop_p = CastChecked<UStructProperty>(property_p);
    const FName struct_name = struct_prop_p->Struct->GetFName();

    if (struct_name == name_Vector2D)     return SkTypeID_Vector2;
    if (struct_name == name_Vector)       return SkTypeID_Vector3;
    if (struct_name == name_Vector4)      return SkTypeID_Vector4;
    if (struct_name == name_Quat)         return SkTypeID_Rotation;
    if (struct_name == name_Rotator)      return SkTypeID_RotationAngles;
    if (struct_name == name_Transform)    return SkTypeID_Transform;
    if (struct_name == name_Color)        return SkTypeID_Color;
    if (struct_name == name_LinearColor)  return SkTypeID_Color;

    return (is_struct_type_supported(struct_prop_p->Struct)) ? SkTypeID_UStruct : SkTypeID_None;
    }

  // enum
  if (const UByteProperty* ByteProperty = Cast<UByteProperty>(property_p))
    return ByteProperty->IsEnum() ? SkTypeID_Enum : SkTypeID_None;

  if (property_p->IsA(UClassProperty::StaticClass()))       return SkTypeID_UClass;

  if (property_p->IsA(UObjectPropertyBase::StaticClass()))
    {
    UClass * class_p = Cast<UObjectPropertyBase>(property_p)->PropertyClass;
    return (does_class_have_static_class(class_p) || class_p->GetName() == TEXT("Object")) ? SkTypeID_UObject : SkTypeID_None;
    }

  if (const UArrayProperty* ArrayProperty = Cast<UArrayProperty>(property_p))
    return (is_property_type_supported(ArrayProperty->Inner) && (get_skookum_property_type(ArrayProperty->Inner) != SkTypeID_List)) ? SkTypeID_List : SkTypeID_None;

  // Didn't find a known type
  return SkTypeID_None;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::get_skookum_property_type_name(UProperty * property_p)
  {
  eSkTypeID type_id = get_skookum_property_type(property_p);
  if (type_id == SkTypeID_UObject)
    {
    UObjectPropertyBase * object_property_p = Cast<UObjectPropertyBase>(property_p);
    m_used_classes.AddUnique(object_property_p->PropertyClass);
    return skookify_class_name(object_property_p->PropertyClass->GetName());
    }

  if (type_id == SkTypeID_UStruct)
    {
    UStruct * struct_p = Cast<UStructProperty>(property_p)->Struct;
    generate_struct(struct_p, m_source_header_file_name);
    return skookify_class_name(struct_p->GetName());
    }

  if (type_id == SkTypeID_Enum)
    {
    UEnum * enum_p = Cast<UByteProperty>(property_p)->Enum;
    return enum_p->GetName();
    }

  return ms_sk_type_id_names[type_id];
  }

//---------------------------------------------------------------------------------------

uint32 FSkookumScriptGenerator::get_skookum_string_crc(const FString & string)
  {
  char buffer[256];
  char * end_p = FPlatformString::Convert(buffer, sizeof(buffer), *string, string.Len());
  return FCrc::MemCrc32(buffer, end_p - buffer);
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::get_cpp_class_name(UStruct * class_or_struct_p)
  {
  return FString::Printf(TEXT("%s%s"), class_or_struct_p->GetPrefixCPP(), *class_or_struct_p->GetName());
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::get_cpp_property_type_name(UProperty * property_p, uint32 port_flags /*= 0*/)
  {
  static FString decl_Enum(TEXT("enum "));
  static FString decl_Struct(TEXT("struct "));
  static FString decl_Class(TEXT("class "));
  static FString decl_TEnumAsByte(TEXT("TEnumAsByte<enum "));
  static FString decl_TSubclassOf(TEXT("TSubclassOf<class "));
  static FString decl_TSubclassOfShort(TEXT("TSubclassOf<"));
  static FString decl_TArray(TEXT("TArray"));

  FString property_type_name = property_p->GetCPPType(NULL, port_flags);
  // Strip any forward declaration keywords
  if (property_type_name.StartsWith(decl_Enum) || property_type_name.StartsWith(decl_Struct) || property_type_name.StartsWith(decl_Class))
    {
    int first_space_index = property_type_name.Find(TEXT(" "));
    property_type_name = property_type_name.Mid(first_space_index + 1);
    }
  else if (property_type_name.StartsWith(decl_TEnumAsByte))
    {
    int first_space_index = property_type_name.Find(TEXT(" "));
    property_type_name = TEXT("TEnumAsByte<") + property_type_name.Mid(first_space_index + 1);
    }
  else if (property_type_name.StartsWith(decl_TSubclassOf)
        || property_type_name.StartsWith(decl_TSubclassOfShort))
    {
    property_type_name = TEXT("UClass *");
    }
  else if (property_type_name.StartsWith(decl_TArray))
    {
    const UArrayProperty* ArrayProperty = Cast<UArrayProperty>(property_p);
    UProperty* InnerProperty = ArrayProperty->Inner;
    property_type_name = FString::Printf(TEXT("TArray<%s>"), *InnerProperty->GetCPPType(nullptr, CPPF_ArgumentOrReturnValue));
    }
  return property_type_name;
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::get_comment_block(UField * field_p)
  {
  // Get tool tip from meta data
  FString comment_block = field_p->GetToolTipText().ToString();
  // Convert to comment block
  if (!comment_block.IsEmpty())
    {
    // "Comment out" the comment block
    comment_block = TEXT("// ") + comment_block;
    comment_block.ReplaceInline(TEXT("\n"), TEXT("\n// "));
    comment_block += TEXT("\n");
    // Replace parameter names with their skookified versions
    for (int32 pos = 0;;)
      {
      pos = comment_block.Find(TEXT("@param"), ESearchCase::IgnoreCase, ESearchDir::FromStart, pos);
      if (pos < 0) break;

      pos += 6; // Skip "@param"
      while (FChar::IsWhitespace(comment_block[pos])) ++pos; // Skip white space
      int32 identifier_begin = pos;
      while (FChar::IsIdentifier(comment_block[pos])) ++pos; // Skip identifier
      int32 identifier_length = pos - identifier_begin;
      // Replace parameter name with skookified version
      FString param_name = skookify_var_name(comment_block.Mid(identifier_begin, identifier_length));
      comment_block.RemoveAt(identifier_begin, identifier_length, false);
      comment_block.InsertAt(identifier_begin, param_name);
      pos += param_name.Len() - identifier_length;
      }
    }

  // Add original name of this object
  FString this_kind =
    field_p->IsA(UFunction::StaticClass()) ? TEXT("method") :
    (field_p->IsA(UClass::StaticClass()) ? TEXT("class") :
    (field_p->IsA(UStruct::StaticClass()) ? TEXT("struct") :
    (field_p->IsA(UProperty::StaticClass()) ? TEXT("property") : TEXT("field"))));
  comment_block += FString::Printf(TEXT("//\n// UE4 name of this %s: %s\n"), *this_kind, *field_p->GetName());

  // Add Blueprint category
  if (field_p->HasMetaData(ms_meta_data_key_function_category))
    {
    FString category_name = field_p->GetMetaData(ms_meta_data_key_function_category);
    comment_block += FString::Printf(TEXT("// Blueprint category: %s\n"), *category_name);
    }

  return comment_block + TEXT("\n");
  }

//---------------------------------------------------------------------------------------

FString FSkookumScriptGenerator::get_skookum_default_initializer(UFunction * function_p, UProperty * param_p)
  {
  FString default_value;
// This is disabled for now until Epic has made some requested changes in HeaderParser.cpp
#if 0
  bool has_default_value = function_p->HasMetaData(*param_p->GetName());
  if (has_default_value)
    {
    default_value = function_p->GetMetaData(*param_p->GetName());
    }
  else
    {
    FName cpp_default_value_key(*(TEXT("CPP_Default_") + param_p->GetName()));
    has_default_value = function_p->HasMetaData(cpp_default_value_key);
    if (has_default_value)
      {
      default_value = function_p->GetMetaData(cpp_default_value_key);
      }
    }
  if (has_default_value)
    {
    // Trivial default?
    if (default_value.IsEmpty())
      {
      eSkTypeID type_id = get_skookum_property_type(param_p);
      switch (type_id)
        {
        case SkTypeID_Integer:         default_value = TEXT("0"); break;
        case SkTypeID_Real:            default_value = TEXT("0.0"); break;
        case SkTypeID_Boolean:         default_value = TEXT("false"); break;
        case SkTypeID_String:          default_value = TEXT("\"\""); break;
        case SkTypeID_Name:            
        case SkTypeID_Vector2:         
        case SkTypeID_Vector3:         
        case SkTypeID_Vector4:         
        case SkTypeID_Rotation:        
        case SkTypeID_RotationAngles:  
        case SkTypeID_Transform:       
        case SkTypeID_Color:           default_value = ms_sk_type_id_names[type_id] + TEXT("!"); break;
        case SkTypeID_UClass:          
        case SkTypeID_UObject:         default_value = skookify_class_name(Cast<UObjectPropertyBase>(param_p)->PropertyClass->GetName()) + TEXT("!null"); break;
        }
      }
    else
      {
      // Remove variable assignments from default_value (e.g. "X=")
      for (int32 pos = 0; pos < default_value.Len(); ++pos)
        {
        if (FChar::IsAlpha(default_value[pos]) && default_value[pos + 1] == '=')
          {
          default_value.RemoveAt(pos, 2);
          }
        }

      // Trim trailing zeros off floating point numbers
      for (int32 pos = 0; pos < default_value.Len(); ++pos)
        {
        if (FChar::IsDigit(default_value[pos]))
          {
          int32 npos = pos;
          while (npos < default_value.Len() && FChar::IsDigit(default_value[npos])) ++npos;
          if (default_value[npos] == '.')
            {
            ++npos;
            while (npos < default_value.Len() && FChar::IsDigit(default_value[npos])) ++npos;
            int32 zpos = npos - 1;
            while (default_value[zpos] == '0') --zpos;
            if (default_value[zpos] == '.') ++zpos;
            ++zpos;
            if (npos > zpos) default_value.RemoveAt(zpos, npos - zpos);
            npos = zpos;
            }
          pos = npos;
          }
        }

      // Skookify the default argument
      eSkTypeID type_id = get_skookum_property_type(param_p);
      switch (type_id)
        {
        case SkTypeID_Integer:         break; // Leave as-is
        case SkTypeID_Real:            break; // Leave as-is
        case SkTypeID_Boolean:         break; // Leave as-is
        case SkTypeID_String:          default_value = TEXT("\"") + default_value + TEXT("\""); break;
        case SkTypeID_Name:            default_value = TEXT("Name!(\"") + default_value + TEXT("\")"); break;
        case SkTypeID_Vector2:         default_value = TEXT("Vector2!xy") + default_value; break;
        case SkTypeID_Vector3:         default_value = TEXT("Vector3!xyz(") + default_value + TEXT(")"); break;
        case SkTypeID_Vector4:         default_value = TEXT("Vector4!xyzw") + default_value; break;
        case SkTypeID_Rotation:        break; // Not implemented yet - leave as-is for now
        case SkTypeID_RotationAngles:  default_value = TEXT("RotationAngles!yaw_pitch_roll(") + default_value + TEXT(")"); break;
        case SkTypeID_Transform:       break; // Not implemented yet - leave as-is for now
        case SkTypeID_Color:           default_value = TEXT("Color!rgba") + default_value; break;
        case SkTypeID_UClass:          break; // Not implemented yet - leave as-is for now
        case SkTypeID_UObject:         if (default_value == TEXT("WorldContext")) default_value = TEXT("@@world"); break;
        }
      }

    default_value = TEXT(" : ") + default_value;
    }
#endif
  return default_value;
  }

//---------------------------------------------------------------------------------------

  FString FSkookumScriptGenerator::get_array_parameter_expression(UProperty * param_p, int32 ParamIndex)
    {
    const UArrayProperty * ArrayProperty = Cast<UArrayProperty>(param_p);
    UProperty * InnerProperty = ArrayProperty->Inner;
    FString inner_cpp_type_name = InnerProperty->GetCPPType(nullptr, CPPF_ArgumentOrReturnValue);
    FString Initializer;
    FString InnerExpression;

    eSkTypeID inner_type_id = get_skookum_property_type(InnerProperty);
    switch (inner_type_id)
      {
      case SkTypeID_Integer:                InnerExpression = TEXT("int(instance->as<SkInteger>())"); break;
      case SkTypeID_Real:                   InnerExpression = TEXT("float(instance->as<SkReal>())"); break;
      case SkTypeID_Boolean:                InnerExpression = TEXT("instance->as<SkBoolean>()"); break;
      case SkTypeID_String:                 InnerExpression = TEXT("FString(instance->as<SkString>().as_cstr())"); break;
      case SkTypeID_Name:                   InnerExpression = TEXT("instance->as<SkUEName>()"); break;
      case SkTypeID_Vector2:                InnerExpression = TEXT("instance->as<SkVector2>()"); break;
      case SkTypeID_Vector3:                InnerExpression = TEXT("instance->as<SkVector3>()"); break;
      case SkTypeID_Vector4:                InnerExpression = TEXT("instance->as<SkVector4>()"); break;
      case SkTypeID_Rotation:               InnerExpression = TEXT("instance->as<SkRotation>()"); break;
      case SkTypeID_RotationAngles:         InnerExpression = TEXT("instance->as<SkRotationAngles>()"); break;
      case SkTypeID_Transform:              InnerExpression = TEXT("instance->as<SkTransform>()"); break;
      case SkTypeID_Color:                  InnerExpression = TEXT("instance->as<SkColor>()"); break;
      case SkTypeID_Enum:                   InnerExpression = FString::Printf(TEXT("(%s)( static_cast<uint8>(instance->as<SkEnum>()) )"), *inner_cpp_type_name); break;
      case SkTypeID_UStruct:                InnerExpression = FString::Printf(TEXT("instance->as<SkUE%s>()"), *get_skookum_property_type_name(InnerProperty)); break;
      case SkTypeID_UClass:                 InnerExpression = TEXT("(UClass*)(instance->as<SkUEEntityClass>())"); break;
      case SkTypeID_UObject:                InnerExpression = FString::Printf(TEXT("instance->as<SkUE%s>()"), *get_skookum_property_type_name(InnerProperty)); break;
      case SkTypeID_List:
      default:                              FError::Throwf(TEXT("Unsupported array inner type: %s"), *InnerProperty->GetClass()->GetName()); break;
      }

    Initializer = FString::Printf( TEXT("    APArray<SkInstance> & param_instances_%d = scope_p->get_arg<SkList>(SkArg_%d).get_instances();\r\n"), ParamIndex + 1, ParamIndex + 1);
    Initializer += FString::Printf(TEXT("    TArray<%s> param_arr_%d;\r\n"), *inner_cpp_type_name, ParamIndex + 1);
    Initializer += FString::Printf(TEXT("    uint32_t param_len_%d = param_instances_%d.get_length();\r\n"), ParamIndex + 1, ParamIndex + 1);
    Initializer += FString::Printf(TEXT("    for (uint32_t i = 0; i < param_len_%d; ++i)\r\n"), ParamIndex + 1);
    Initializer +=                 TEXT("      {\r\n");
    Initializer += FString::Printf(TEXT("      SkInstance * instance = param_instances_%d[i];\r\n"), ParamIndex + 1);
    Initializer += FString::Printf(TEXT("      param_arr_%d.Add(%s);\r\n"), ParamIndex + 1, *InnerExpression);
    Initializer +=                 TEXT("      }\r\n");
    return Initializer;
    }

//---------------------------------------------------------------------------------------

bool FSkookumScriptGenerator::save_header_if_changed(const FString & header_path, const FString & new_header_contents)
  {
  FString original_header_local;
  FFileHelper::LoadFileToString(original_header_local, *header_path);

  const bool has_changed = original_header_local.Len() == 0 || FCString::Strcmp(*original_header_local, *new_header_contents);
  if (has_changed)
    {
    // save the updated version to a tmp file so that the user can see what will be changing
    const FString temp_header_file_name = header_path + TEXT(".tmp");

    // delete any existing temp file
    IFileManager::Get().Delete(*temp_header_file_name, false, true);
    if (!FFileHelper::SaveStringToFile(new_header_contents, *temp_header_file_name))
      {
      UE_LOG(LogSkookumScriptGenerator, Warning, TEXT("Failed to save header export: '%s'"), *temp_header_file_name);
      }
    else
      {
      m_temp_headers.Add(temp_header_file_name);
      }
    }

  return has_changed;
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::rename_temp_files()
  {
  // Rename temp headers
  for (auto & temp_header_file_name : m_temp_headers)
    {
    FString file_name = temp_header_file_name.Replace(TEXT(".tmp"), TEXT(""));
    if (!IFileManager::Get().Move(*file_name, *temp_header_file_name, true, true))
      {
      UE_LOG(LogSkookumScriptGenerator, Error, TEXT("%s"), *FString::Printf(TEXT("Couldn't write file '%s'"), *file_name));
      }
    else
      {
      UE_LOG(LogSkookumScriptGenerator, Log, TEXT("Exported updated script header: %s"), *file_name);
      }
    }
  }

//=======================================================================================
// MethodBinding implementation
//=======================================================================================

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::MethodBinding::make_method(UFunction * function_p)
  {
  m_script_name = skookify_method_name(function_p->GetName(), function_p->GetReturnProperty());
  m_code_name = m_script_name.Replace(TEXT("?"), TEXT("_Q"));
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::MethodBinding::make_property_getter(UProperty * property_p)
  {
  m_script_name = skookify_method_name(property_p->GetName(), property_p);
  m_code_name = m_script_name.Replace(TEXT("?"), TEXT("_Q"));
  }

//---------------------------------------------------------------------------------------

void FSkookumScriptGenerator::MethodBinding::make_property_setter(UProperty * property_p)
  {
  m_script_name = skookify_method_name(property_p->GetName()) + TEXT("_set");
  m_code_name = m_script_name;
  }
