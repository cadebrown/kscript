
/* ks.js - JavaScript bindings for interfacing with kscript
 * 
 * You should include two files:
 *   * This file, which is the javascript bindings
 *   * 'libks.js', the compiled kscript library
 * 
 * For example, in HTML you can do:
 * 
 * ```html
 * <script src="../../lib/libks.js"></script>
 * <script src="ks.js"></script>
 * ```
 * 
 * kscript hosts versions online, so you can have (for example):
 * 
 * ```html
 * <script src="https://kscript.org/dist/web/v0.0.1/libks.js"></script>
 * <script src="https://kscript.org/dist/web/v0.0.1/ks.js"></script>
 * ```
 * 
 * Once these are included, 
 * 
 * 
 *
 * Things are still reference counted. When using the exported API (i.e. functions without '_' prefixed),
 *   these should either return JS primitives, or a 'ks.Object'. If the given method says it 'returns
 *   a reference' that means it returns a ks.Object with an active reference that you need to take away
 *   when you are finished using the object. You can do that with 'obj.decref()'
 *
 * For example:
 * ```
 * var x = ks.eval("2 + 3");
 * console.log(x.toString());
 * x.decref(); // done with x
 * ```
 *
 * Or, if you don't care about the return value at all, just go ahead and decref:
 * `ks.eval("2 + 3").decref()`
 *
 * SEE: https://emscripten.org/docs/api_reference/module.html?highlight=stdout#Module.print
 * 
 * @author: Cade Brown <cade@kscript.org>
 */


var ks = null;
libks().then(function (MOD) {
    ks = MOD;

    /* Enums/Constants */

    ks.LogLevel = {
        Trace:     10,
        Debug:     20,
        Info :     30,
        Warn :     40,
        Error:     50,
        Fatal:     60,
    }

    /* Types */

    ks.Error = class {
        constructor(_msg, _objs=[]) {
            this._msg = _msg.toString();
            this._objs = _objs;
        }
    }

    ks.Object = class {
        constructor(_obj) {
            if (_obj == undefined) return; //throw Error("Given 'undefined' to Object constructor");
            if (_obj instanceof ks.Object) {
                return _obj.newref();
            } else if (typeof _obj === 'string') {
                var str = _obj.toString();

                // fill a buffer with the UTF8 bytes of the string
                var buf = ks._malloc(str.length + 1);
                ks.stringToUTF8(str, buf, str.length + 1);

                // wrap the result as a new object
                var res = ks._str_new(str.length, buf);

                // free temporary buffer
                ks._free(buf);
                return ks.Object.wrap(res);

            } else if (typeof _obj === 'number') {
                // TODO: how to specify integral or not? by default we use float
                return ks.Object.wrap(ks._float_new(_obj));
            } else {
                throw new ks.Error("Invalid object given: " + _obj.toString() + " (bad type: " + typeof _obj + ")", [_obj]);
            }
        }

        // wrap a pointer as an object
        static wrap(_ptr) {
            // if it is NULL, then check for errors
            if (!_ptr) {
                ks.check();
            }

            // create a new empty object and set the pointer
            var res = new ks.Object();
            res._ptr = _ptr;
            return res;
        }
        
        // return the type of the object
        type() {
            return ks.Object.wrap(ks._type(this._ptr));
        }

        // return the number of references to the object
        refs() {
            return ks._refs(this._ptr);
        }

        // increase the reference count of the object
        incref() {
            ks._incref(this._ptr);
        }

        // decrease the reference count of the object
        // TODO: detect if freed
        decref() {
            ks._decref(this._ptr);
        }

        // returns a new reference to the current object
        newref() {
            this.incref();
            return this;
        }

        // convert to a repr
        // NOTE: Returns JS string
        toRepr() {
            var buf = ks._repr_c_(this._ptr);
            ks.check();

            var res = ks.UTF8ToString(buf);
            ks._free(buf);
            return res;
        }

        // convert to a usable string
        // NOTE: Returns JS string
        toString() {
            var buf = ks._str_c_(this._ptr);
            ks.check();
            
            var res = ks.UTF8ToString(buf);
            ks._free(buf);
            return res;
        }

        // the 'value' of the object, which is the pointer representing
        //   the address of it (also its ID)
        // This is so decaying to a number works fine
        valueOf() {
            return this._ptr;
        }

        // Returns `self.attr`
        // NOTE: Returns a new reference
        getattr() {
            // transform into a list of references
            var args = [this].concat(...arguments);
            for (var i = 0; i < args.length; ++i) {
                var arg = args[i];
                if (typeof arg === ks.Object) {
                    arg.incref();
                } else {
                    args[i] = new ks.Object(arg);
                }
            }

            var args_array = ks._array_of_ptrs(args);
            var res = ks._call(ks.globals.__getattr__._ptr, args_array, args.length);
            ks._free(args_array);

            // destroy references
            for (var i = 0; i < args.length; ++i) args[i].decref();

            ks.check();
            return ks.Object.wrap(res);
        }

        // returns `self[*args]`
        getitem() {
            // transform into a list of references
            var args = [this].concat(...arguments);
            for (var i = 0; i < args.length; ++i) {
                var arg = args[i];
                if (typeof arg === ks.Object) {
                    arg.incref();
                } else {
                    args[i] = new ks.Object(arg);
                }
            }

            var args_array = ks._array_of_ptrs(args);
            var res = ks._call(ks.globals.__getitem__._ptr, args_array, args.length);
            ks._free(args_array);

            // destroy references
            for (var i = 0; i < args.length; ++i) args[i].decref();

            ks.check();
            return ks.Object.wrap(res);
        }

        // returns `self(*args)`
        call() {
            // transform into a list of references
            var args = [].concat(...arguments);
            for (var i = 0; i < args.length; ++i) {
                var arg = args[i];
                if (typeof arg === ks.Object) {
                    arg.incref();
                } else {
                    args[i] = new ks.Object(arg);
                }
            }

            var args_array = ks._array_of_ptrs(args);
            var res = ks._call(this._ptr, args_array, args.length);
            ks._free(args_array);

            // destroy references
            for (var i = 0; i < args.length; ++i) args[i].decref();

            return ks.Object.wrap(res);
        }
    }

    // javascript logging utility
    ks.jslog = function(...args) {
        console.log("[kscript]", ...args)
    }

    /* Low Level C API (prefixed with '_') */

    var void_t = null, obj_t = 'number', bool_t = 'number', ptr_t = 'number', int_t = 'number', float_t = 'number';
    ks._init = ks.cwrap('ks_init', bool_t, [])
    //ks._finalize = ks.cwrap('ks_finalize', void_t, []);

    ks._exit_if_err = ks.cwrap('kso_exit_if_err', void_t, [])
    ks._catch_ignore = ks.cwrap('kso_catch_ignore', bool_t, [])
    ks._catch_ignore_print = ks.cwrap('kso_catch_ignore_print', bool_t, [])

    ks._str_new = ks.cwrap('ks_str_new', obj_t, [int_t, ptr_t]);

    // override memory functions
    ks._malloc = ks.cwrap('ks_malloc', ptr_t, [int_t]);
    ks._free = ks.cwrap('ks_free', void_t, [ptr_t]);
    ks._eval = ks.cwrap('kso_eval', obj_t, [obj_t, obj_t, obj_t]);

    ks._incref = ks.cwrap('_ksem_incref_', void_t, [obj_t]);
    ks._decref = ks.cwrap('_ksem_decref_', void_t, [obj_t]);
    ks._refs = ks.cwrap('_ksem_refs_c_', int_t, [obj_t]);
    ks._type = ks.cwrap('_ksem_type_c_', obj_t, [obj_t]);
    //ks._get_ptr_size = ks.cwrap('ks_get_ptr_size', int_t, []);

    ks._str_c_ = ks.cwrap('_ksem_str_c_', ptr_t, [obj_t]);
    ks._repr_c_ = ks.cwrap('_ksem_repr_c_', ptr_t, [obj_t]);

    ks._dict_new = ks.cwrap('ks_dict_new', obj_t, [int_t, ptr_t]);
    ks._get_verstr = ks.cwrap('ks_get_verstr', ptr_t, []);


    // list of global builtins
    ks.globals = {};

    // returns an array of pointers for 'args'
    // NOTE: free the result via `ks._free()`
    ks._array_of_ptrs = function (args) {
        if (ks.SIZE_PTR != 4) {
            ks.jslog("ks.SIZE_PTR (" + ks.SIZE_PTR.toString + ") was not 4, so things will probably be wrong (or weird)")
        }

        var args_array = new Uint32Array(args.length);

        for (var i = 0; i < args.length; ++i) {
            args_array[i] = args[i].valueOf();
        }

        var buf_sz = args_array.length * args_array.BYTES_PER_ELEMENT
        var buf = ks._malloc(buf_sz)
        var buf_heap = new Uint32Array(ks.HEAPU8.buffer, buf, args_array.length)

        buf_heap.set(args_array);

        return buf;
    }


    /* High Level API (no prefix) */

    // check for any errors
    ks.check = function () {
        // TODO; throw ks.Error with a message for it
        //ks._exit_if_err();
        //ks._catch_ignore();
        ks._catch_ignore_print();
    }

    // evaluate a string
    // NOTE: Returns a new reference
    ks.eval = function (expr, fname="<eval>", locals=0) {
        if (typeof expr === 'string') expr = new ks.Object(expr);
        else if (typeof expr === ks.Object) expr.incref();
        else {
            throw new ks.Error("Cannot 'eval' object of type " + typeof expr, [expr]);
        }
        if (typeof fname === ks.Object) fname.incref();
        else fname = new ks.Object(fname.toString());

        var res = ks._eval(expr._ptr, fname._ptr, locals || locals._ptr);
        expr.decref();
        fname.decref();

        ks.check();
        return res && ks.Object.wrap(res);
    }


    /* Now, initialize the library */
    if (!ks._init()) {
        ks.jslog("Failed to initialize libks")
    } else {
        ks.jslog("Initialized libks")
    }
    




    /*
    if (!ks._init(ks.LogLevel.Debug)) {
        ks.jslog("failed to initialize libks!")
    } else {

        ks.eval("print(234)");

        return
        ks.globals.__getitem__ = ks.Object.wrap(ks._get_getitem());
        ks.globals.__getattr__ = ks.Object.wrap(ks._get_getattr());
        const ks_globals = ks.Object.wrap(ks._get_globals());

        // initialize builtins
        var builtin_names = ["bool", "int", "float", "complex", "str", "bytes", "list", "tuple", "set", "dict", "graph", "object", "type", "attrtuple", "namespace", "func", "logger", "range", "slice", "print", "repr", "iter", "next", "chr", "ord", "id", "len", "hash"];

        for (var i = 0; i < builtin_names.length; ++i) {
            var key = builtin_names[i];
            ks.globals[key] = ks_globals.getitem(key);
        }

        ks_globals.decref();
    }
    */
})
