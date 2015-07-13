/*
 // Copyright (c) 2015 Eliott Paris, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "MaxV8.h"

namespace cicm
{
    //============================================================================
    // Global Variables
    //============================================================================
    t_class* MaxV8::obj_class = nullptr;
    
    Platform* MaxV8::v8_platform;
    
    class ArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
    public:
        virtual void* Allocate(size_t length) {
            void* data = AllocateUninitialized(length);
            return data == NULL ? data : memset(data, 0, length);
        }
        virtual void* AllocateUninitialized(size_t length) { return malloc(length); }
        virtual void Free(void* data, size_t) { free(data); }
    };
    
    //============================================================================
    // MaxV8 public Methods
    //============================================================================
    
    void MaxV8::Init()
    {
        // Initialize V8.
        V8::InitializeICU();
        v8_platform = platform::CreateDefaultPlatform();
        V8::InitializePlatform(v8_platform);
        V8::Initialize();
        
        post("v8 version : %s", V8::GetVersion());
    }
    
    void MaxV8::Release()
    {
        V8::Dispose();
        V8::ShutdownPlatform();
        delete v8_platform;
    }
    
    bool MaxV8::InitInstance(Isolate* isolate, long argc, t_atom* argv)
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = CreateMaxContext(isolate);
        m_js_context.Reset(isolate, context);
        
        if (argc > 0 && atom_gettype(argv) == A_SYM)
        {
            const char* file = atom_getsym(argv)->s_name;
            
            Local<v8::String> script = ReadFile(file);
            if (!script.IsEmpty())
            {
                // Enter the new context so all the following operations take place within it.
                v8::Context::Scope context_scope(context);
                ExecuteScript(script);
            }
        }
        
        return true;
    }
    
    v8::Local<v8::String> MaxV8::ReadFile(const char* filename)
    {
        v8::Isolate* isolate = v8::Isolate::GetCurrent();
        EscapableHandleScope handle_scope(isolate);
        
        t_fourcc type = FOUR_CHAR_CODE('TEXT');
        
        strncpy_zero(m_filename, filename, MAX_FILENAME_CHARS);
        
        if(locatefile_extended(m_filename, &m_path, &type, &type, 1))
        {
            object_error((t_object *)&obj, "can't find file %s", m_filename);
            return handle_scope.Escape(Local<v8::String>());
        }
        
        t_filehandle fh;
        short err = path_opensysfile(m_filename, m_path, &fh, READ_PERM);
        
        if (!err)
        {
            sysfile_readtextfile(fh, m_text, 0, (t_sysfile_text_flags) (TEXT_LB_NATIVE | TEXT_NULL_TERMINATE));
            sysfile_close(fh);
            m_textsize = sysmem_handlesize(m_text);
            
            if(m_text)
            {
                return handle_scope.Escape(v8::String::NewFromUtf8(isolate, *m_text));
            }
        }
        
        return handle_scope.Escape(Local<v8::String>());
    }
    
    Local<v8::Context> MaxV8::CreateMaxContext(v8::Isolate* isolate)
    {
        // Create a template for the global object.
        Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
        
        // Bind the global 'post' function to the C++ post callback.
        global->Set(v8::String::NewFromUtf8(isolate, "post"),
                    v8::FunctionTemplate::New(isolate, JsPost));
        
        // Bind the global 'object_post' function to the C++ post callback.
        global->Set(v8::String::NewFromUtf8(isolate, "object_post"),
                    v8::FunctionTemplate::New(isolate, JsObjectPost));
        
        // Bind the global 'error' function to the C++ error callback.
        global->Set(v8::String::NewFromUtf8(isolate, "error"),
                    v8::FunctionTemplate::New(isolate, JsError));
        
        // Wrap the raw t_object pointer in an External so it can be referenced
        // from within JavaScript.
        Local<External> obj_ptr = External::New(isolate, &obj);
        global->SetAccessor(String::NewFromUtf8(isolate, "inlets"), JsInletsGetter, JsInletsSetter, obj_ptr);
        global->SetAccessor(String::NewFromUtf8(isolate, "outlets"), JsOutletsGetter, JsOutletsSetter, obj_ptr);
        
        // jsarguments getter
        global->SetAccessor(String::NewFromUtf8(isolate, "jsarguments"), JsArgumentsGetter, nullptr, obj_ptr);
        
        // proxy_getinlet wrapper
        global->SetAccessor(String::NewFromUtf8(isolate, "inlet"), JsProxyInletsGetter, nullptr, obj_ptr);
        
        // Bind the global 'outlet' function to the C++ callback.
        global->Set(v8::String::NewFromUtf8(isolate, "outlet"),
                    v8::FunctionTemplate::New(isolate, JsOutput, obj_ptr));
        
        // Bind the global 'setinletassist' function to the C++ callback.
        global->Set(v8::String::NewFromUtf8(isolate, "setinletassist"),
                    v8::FunctionTemplate::New(isolate, JsSetInletAssist, obj_ptr));
        
        // Bind the global 'setoutletassist' function to the C++ callback.
        global->Set(v8::String::NewFromUtf8(isolate, "setoutletassist"),
                    v8::FunctionTemplate::New(isolate, JsSetOutletAssist, obj_ptr));
        
        return v8::Context::New(isolate, NULL, global);
    }
    
    Local<Value> MaxV8::ExecuteScript(v8::Handle<v8::String> script)
    {
        EscapableHandleScope handle_scope(m_isolate);
        
        // We're just about to compile the script; set up an error handler to
        // catch any exceptions the script might throw.
        v8::TryCatch try_catch;
        
        // Compile the script and check for errors
        Local<Script> compiled_script = Script::Compile(script);
        if (compiled_script.IsEmpty())
        {
            v8::String::Utf8Value error_string(try_catch.Exception());
            object_error((t_object*)&obj, "Compilation error: %s", *error_string);
            // The script failed to compile; bail out.
            return handle_scope.Escape(Local<Value>());
        }
        
        // Run the script
        Local<Value> result = compiled_script->Run();
        if (result.IsEmpty())
        {
            // The TryCatch above is still in effect and will have caught the error.
            v8::String::Utf8Value error_string(try_catch.Exception());
            object_error((t_object*)&obj, "Script error: %s", *error_string);
            // Running the script failed; bail out.
            return handle_scope.Escape(Local<Value>());
        }
        
        return handle_scope.Escape(result);
    }
    
    //============================================================================
    // MaxV8 Methods called by Max
    //============================================================================
    void* MaxV8::NewInstance(t_symbol* s, long argc, t_atom* argv)
    {
        MaxV8* x = (MaxV8*)object_alloc(obj_class);
        
        if (x)
        {
            object_post((t_object*)&x->obj, "new v8 instance");
            
            x->m_text = sysmem_newhandle(0);
            x->m_textsize = 0;
            x->m_texteditor = nullptr;
            
            x->m_obj_argc = argc;
            if(argc)
            {
                x->m_obj_argv = new t_atom[argc];
                for(long i = 0; i < argc; i++)
                {
                    switch (atom_gettype(argv+i))
                    {
                        case A_LONG:    atom_setlong(x->m_obj_argv+i, atom_getlong(argv+i)); break;
                        case A_FLOAT:   atom_setfloat(x->m_obj_argv+i, atom_getfloat(argv+i)); break;
                        case A_SYM:     atom_setsym(x->m_obj_argv+i, atom_getsym(argv+i)); break;
                        default: break;
                    }
                }
            }
            
            // Create a new Isolate and make it the current one.
            ArrayBufferAllocator allocator;
            Isolate::CreateParams create_params;
            create_params.array_buffer_allocator = &allocator;
            x->m_isolate = Isolate::New(create_params);
            
            x->m_number_of_inlets = 1;
            
            if (!x->InitInstance(x->m_isolate, argc, argv))
            {
                object_free(x);
                x = nullptr;
            }
        }
        
        return x;
    }
    
    void MaxV8::FreeInstance(MaxV8* x)
    {
        if (x->m_text)
            sysmem_freehandle(x->m_text);
        
        if(x->m_obj_argc > 1)
        {
            delete [] x->m_obj_argv;
        }
        
        //x->m_js_context.Reset();
        
        // need to dispose isolate but crash for now !
        //x->m_isolate->Dispose();
    }
    
    void MaxV8::Assist(MaxV8* x, void* b, long io_type, long index, char* s)
    {
        if (io_type == ASSIST_INLET)
        {
            auto it = x->m_inlet_assist.find(index);
            if(it != x->m_inlet_assist.end())
            {
                string text = it->second;
                sprintf(s, "%s", text.c_str());
            }
            else
            {
                sprintf(s, "%s: Inlet %ld", x->m_filename, index);
            }
        }
        else
        {
            auto it = x->m_outlet_assist.find(index);
            if(it != x->m_outlet_assist.end())
            {
                string text = it->second;
                sprintf(s, "%s", text.c_str());
            }
            else
            {
                sprintf(s, "%s: Outlet %ld", x->m_filename, index);
            }
        }
    }
    
    void MaxV8::Read(MaxV8* x, t_symbol *s)
    {
        defer((t_object *)x, (method)DoRead, s, 0, NULL);
    }
    
    void MaxV8::DoRead(MaxV8* x, t_symbol *s, long argc, t_atom *argv)
    {
        char filename[MAX_PATH_CHARS];
        short path;
        t_fourcc type = FOUR_CHAR_CODE('JSON');
        
        if (s == gensym(""))
        {
            filename[0] = 0;
            
            if (open_dialog(filename, &path, &type, &type, 1))
                return;
        }
        else
        {
            strcpy(filename, s->s_name);
            
            if(locatefile_extended(filename, &path, &type, &type, 1))
            {
                object_error((t_object *)x, "can't find file %s", filename);
                return;
            }
        }
    }
    
    void MaxV8::OpenEditor(MaxV8* x)
    {
        if (x->m_texteditor)
        {
            object_attr_setchar(x->m_texteditor, gensym("visible"), 1);
        }
        else
        {
            x->m_texteditor = (t_object*) object_new(CLASS_NOBOX, gensym("jed"), x, 0);
            object_method(x->m_texteditor, gensym("settext"), *x->m_text, gensym("utf-8"));
            object_method(x->m_texteditor, gensym("filename"), x->m_filename, x->m_path);
            
            //object_attr_setchar(x->m_texteditor, gensym("scratch"), 1);
            //object_attr_setsym(x->m_texteditor, gensym("title"), gensym("v8 editor"));
        }
    }
    
    void MaxV8::EditorClosed(MaxV8* x, char **text, long size)
    {
        if (x->m_text)
            sysmem_freehandle(x->m_text);
        
        x->m_text = sysmem_newhandleclear(size+1);
        sysmem_copyptr((char *)*text, *x->m_text, size);
        x->m_textsize = size+1;
        x->m_texteditor = nullptr;
    }
    
    long MaxV8::EditorSaved(MaxV8* *x, char **text, long size)
    {
        post("doc saved !!");
        
        return 0; // tell editor it can save the text
    }
    
    void MaxV8::Loadbang(MaxV8* x)
    {
        post("(cpp) : loadbang called");
        CallJsFunction(x, gensym("loadbang"), 0, nullptr);
    }
    
    void MaxV8::Bang(MaxV8* x)
    {
        post("(cpp) : bang received");
        CallJsFunction(x, gensym("bang"), 0, nullptr);
    }
    
    void MaxV8::Anything(MaxV8* x, t_symbol *s, long ac, t_atom *av)
    {
        post("(cpp) : Anything method (%s)", s->s_name);
        CallJsFunction(x, s, ac, av);
    }
    
    void MaxV8::Int(MaxV8* x, long number)
    {
        post("(cpp) : int received %ld", number);
        t_atom av[1];
        atom_setlong(av, number);
        CallJsFunction(x, gensym("msg_int"), 1, av);
    }
    
    void MaxV8::Float(MaxV8* x, double number)
    {
        post("(cpp) : double received %f", (float)number);
        
        t_atom av[1];
        atom_setfloat(av, number);
        CallJsFunction(x, gensym("msg_float"), 1, av);
    }
    
    void MaxV8::resizeInlets(MaxV8 *x)
    {
        t_object *b = NULL;
        object_obex_lookup(x, gensym("#B"), (t_object **)&b);
        
        if (b)
        {
            object_method(b, gensym("dynlet_begin"));
            
            if(inlet_count((t_object *)x) > m_number_of_inlets)
            {
                for(int i = inlet_count((t_object *)x); i > m_number_of_inlets; i--)
                {
                    proxy_delete(inlet_nth((t_object*)x, i-1));
                }
            }
            else if(inlet_count((t_object *)x) < m_number_of_inlets)
            {
                for(int i = inlet_count((t_object *)x); i < m_number_of_inlets; i++)
                {
                    proxy_append((t_object*)x, i, &m_inletcount);
                }
            }
            
            object_method(b, gensym("dynlet_end"));
        }
    }
    
    void MaxV8::resizeOutlets(MaxV8 *x)
    {
        t_object *b = NULL;
        object_obex_lookup(x, gensym("#B"), (t_object **)&b);
        
        if (b)
        {
            object_method(b, gensym("dynlet_begin"));
            
            if(outlet_count((t_object *)x) > m_number_of_outlets)
            {
                for(int i = outlet_count((t_object *)x); i > m_number_of_outlets; i--)
                {
                    outlet_delete(outlet_nth((t_object*)x, i-1));
                    m_outlets.pop_back();
                }
            }
            else if(outlet_count((t_object *)x) < m_number_of_outlets)
            {
                for(int i = outlet_count((t_object *)x); i < m_number_of_outlets; i++)
                {
                    m_outlets.push_back(outlet_append((t_object*)x, NULL, NULL));
                }
            }
            
            object_method(b, gensym("dynlet_end"));
        }
    }
    
    //============================================================================
    // v8 Handles
    //============================================================================
    
    void MaxV8::CallJsFunction(MaxV8* x, t_symbol *s, long ac, t_atom *av)
    {
        Isolate* isolate = x->m_isolate;
        HandleScope handle_scope(isolate);
        
        Local<v8::Context> context = Local<v8::Context>::New(isolate, x->m_js_context);
        
        Local<v8::Object> global = context->Global();
        
        MaybeLocal<Value> value = global->Get(context, v8::String::NewFromUtf8(isolate, s->s_name));
        
        if(!value.IsEmpty())
        {
            Local<Value> local_value = value.ToLocalChecked();
            
            if (local_value->IsFunction())
            {
                Local<v8::Function> fn = v8::Local<v8::Function>::Cast(local_value);
                
                Local<Value>* args = new Local<Value>[ac];
                for(long i = 0; i < ac; i++)
                {
                    switch (atom_gettype(av+i))
                    {
                        case A_LONG:  args[i] = v8::Integer::New(isolate, atom_getlong(av+i)); break;
                        case A_FLOAT: args[i] = v8::Number::New(isolate, atom_getfloat(av+i));break;
                        case A_SYM:   args[i] = v8::String::NewFromUtf8(isolate, atom_getsym(av+i)->s_name); break;
                        default: break;
                    }
                }
                fn->Call(context, fn, ac, args);
            }
        }
    }
    
    void MaxV8::JsArgumentsGetter(Local<String> property, const PropertyCallbackInfo<Value>& info)
    {
        Isolate* isolate = info.GetIsolate();
        
        // We will be creating temporary handles so we use a handle scope.
        HandleScope handle_scope(isolate);
        
        Local<External> data = Local<External>::Cast(info.Data());
        MaxV8* x = static_cast<MaxV8*>(data->Value());
        
        if(x->m_obj_argc)
        {
            // Create a new empty array.
            Local<Array> array = Array::New(isolate, x->m_obj_argc);
            
            // Return an empty result if there was an error creating the array.
            if (!array.IsEmpty())
            {
                for(long i = 0; i < x->m_obj_argc; i++)
                {
                    switch (atom_gettype(x->m_obj_argv+i))
                    {
                        case A_LONG:  array->Set(i, Integer::New(isolate, atom_getlong(x->m_obj_argv+i))); break;
                        case A_FLOAT: array->Set(i, Number::New(isolate, atom_getfloat(x->m_obj_argv+i))); break;
                        case A_SYM:   array->Set(i, String::NewFromUtf8(isolate, atom_getsym(x->m_obj_argv+i)->s_name)); break;
                        default: break;
                    }
                }
                
                info.GetReturnValue().Set(array);
                return;
            }
        }
        
        info.GetReturnValue().Set(Local<Array>());
    }
    
    void MaxV8::JsProxyInletsGetter(Local<String> property, const PropertyCallbackInfo<Value>& info)
    {
        Local<External> data = Local<External>::Cast(info.Data());
        MaxV8* x = static_cast<MaxV8*>(data->Value());
        
        long inlet = proxy_getinlet((t_object*)x);
        
        Local<Number> l_inlet = Number::New(info.GetIsolate(), inlet);
        
        info.GetReturnValue().Set(l_inlet);
    }
    
    void MaxV8::JsInletsGetter(Local<String> property, const PropertyCallbackInfo<Value>& info)
    {
        Local<External> data = Local<External>::Cast(info.Data());
        MaxV8* x = static_cast<MaxV8*>(data->Value());
        
        info.GetReturnValue().Set(x->m_number_of_inlets);
    }
    
    void MaxV8::JsInletsSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
    {
        Local<External> data = Local<External>::Cast(info.Data());
        MaxV8* x = static_cast<MaxV8*>(data->Value());
        
        int ins = value->Int32Value();
        if(ins < 0) ins = 0;
        if(ins > 250) ins = 250;
        
        if(x->m_number_of_inlets != ins)
        {
            x->m_number_of_inlets = ins;
            x->resizeInlets(x);
        }
    }
    
    void MaxV8::JsOutletsGetter(Local<String> property, const PropertyCallbackInfo<Value>& info)
    {
        Local<External> data = Local<External>::Cast(info.Data());
        MaxV8* x = static_cast<MaxV8*>(data->Value());
        
        info.GetReturnValue().Set(x->m_number_of_outlets);
    }
    
    void MaxV8::JsOutletsSetter(Local<String> property, Local<Value> value, const PropertyCallbackInfo<void>& info)
    {
        Local<External> data = Local<External>::Cast(info.Data());
        MaxV8* x = static_cast<MaxV8*>(data->Value());
        
        int outs = value->Int32Value();
        if(outs < 0) outs = 0;
        if(outs > 250) outs = 250;
        
        if(x->m_number_of_outlets != outs)
        {
            x->m_number_of_outlets = outs;
            x->resizeOutlets(x);
        }
    }
    
    void MaxV8::JsOutput(FunctionCallbackInfo<Value> const& args)
    {
        Isolate* isolate = args.GetIsolate();
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);
        
        if (args.Length() < 2)
        {
            isolate->ThrowException(String::NewFromUtf8(isolate, "Bad parameters"));
            return;
        }
        
        Local<External> data = Local<External>::Cast(args.Data());
        MaxV8* x = static_cast<MaxV8*>(data->Value());
        
        long index = 0;
        
        Maybe<double> may_num = args[0]->NumberValue(isolate->GetCurrentContext());
        if(may_num.IsJust())
        {
            index = may_num.FromJust();
            if (index < 0 || index >= x->m_number_of_inlets)
            {
                isolate->ThrowException(String::NewFromUtf8(isolate, "Bad index"));
                return;
            }
        }
        else
        {
            isolate->ThrowException(String::NewFromUtf8(isolate, "Bad index"));
            return;
        }
        
        short argc = 0;
        t_atom* argv = new t_atom[args.Length() - 1];
        for(int i = 0; i < args.Length() - 1; i++)
        {
            Local<Value> arg = args[i+1];
            
            if(arg->IsNumber())
            {
                Maybe<double> val = arg->NumberValue(isolate->GetCurrentContext());
                if(val.IsJust())
                {
                    atom_setfloat(argv+argc, val.FromJust());
                    argc++;
                }
            }
            else if(arg->IsString())
            {
                v8::String::Utf8Value str(arg);
                atom_setsym(argv+argc, gensym(ToCString(str)));
                argc++;
            }
        }
        
        if(argc > 1)
        {
            outlet_anything(x->m_outlets[index], gensym("list"), argc, argv);
        }
        
        delete [] argv;
    }
    
    void MaxV8::JsSetInletAssist(FunctionCallbackInfo<Value> const& args)
    {
        /*
         Isolate* isolate = args.GetIsolate();
         v8::Isolate::Scope isolate_scope(isolate);
         HandleScope handle_scope(isolate);
         
         if (args.Length() != 2)
         {
         isolate->ThrowException(String::NewFromUtf8(isolate, "Bad parameters"));
         return;
         }
         
         Local<External> data = Local<External>::Cast(args.Data());
         MaxV8* x = static_cast<MaxV8*>(data->Value());
         
         int index = args[0]->Int32Value();
         if (index >= x->m_number_of_inlets)
         {
         isolate->ThrowException(String::NewFromUtf8(isolate, "Bad index"));
         return;
         }
         
         
         v8::String::Utf8Value str(args[1]);
         const char* cstr = ToCString(str);
         
         //this crash
         x->m_inlet_assist[1] = "fefefefe";
         */
        
        //post("(c++) inlet assist %ld : %s", index, cstr);
    }
    
    void MaxV8::JsSetOutletAssist(const FunctionCallbackInfo<Value>& args)
    {
        /*
         Isolate* isolate = args.GetIsolate();
         HandleScope handle_scope(isolate);
         
         if (args.Length() != 2)
         {
         isolate->ThrowException(String::NewFromUtf8(isolate, "Bad parameters"));
         return;
         }
         
         Local<External> data = Local<External>::Cast(args.Data());
         MaxV8* x = static_cast<MaxV8*>(data->Value());
         
         int index = args[0]->Int32Value();
         if (index >= x->m_number_of_outlets)
         {
         isolate->ThrowException(String::NewFromUtf8(isolate, "Bad index"));
         return;
         }
         
         String::Utf8Value utf8_str(args[1]);
         std::string ascii_str(*utf8_str ? *utf8_str : "<unknown string>");
         x->m_outlet_assist[index] = ascii_str;
         */
    }
    
    //============================================================================
    // v8 Handles (post, error...)
    //============================================================================
    
    const char* MaxV8::ToCString(String::Utf8Value const& value)
    {
        return *value ? *value : "<string conversion failed>";
    }
    
    const char* MaxV8::getPostCString(FunctionCallbackInfo<Value> const& args)
    {
        Isolate::Scope isolate_scope(args.GetIsolate());
        string postStr;
        for(int i = 0; i < args.Length(); i++)
        {
            HandleScope handle_scope(args.GetIsolate());
            if (i > 0)
            {
                postStr.append(" ");
            }
            
            if(!args[i].IsEmpty())
            {
                v8::String::Utf8Value str(args[i]);
                const char* cstr = ToCString(str);
                postStr.append(cstr);
            }
        }
        
        return postStr.c_str();
    }
    
    void MaxV8::JsPost(FunctionCallbackInfo<Value>const& args)
    {
        post(getPostCString(args));
    }
    
    void MaxV8::JsObjectPost(FunctionCallbackInfo<Value>const& args)
    {
        post(getPostCString(args));
    }
    
    void MaxV8::JsError(FunctionCallbackInfo<Value>const& args)
    {
        error(getPostCString(args));
    }
}
