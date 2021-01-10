/* ks/kscxx.hh - kscript C++ utilities
 * 
 * This file can be used to utilize a C++-friendly interface.
 * 
 * Especially helpful for when some C-constructs don't compile in C++, such as temporary arrays,
 *   which kscript constructors use (i.e. KS_II or KS_IKV). These functions are in the namespace 'ks',
 *   and are the 'make_*' functions. For example, 'ks::make_str' turns a C++ string into a kscript 'str'
 *   object
 * 
 * @author:    Cade Brown <cade@kscript.org>
 * @license:   GPLv3
 */

#pragma once
#ifndef KSCXX_HH__
#define KSCXX_HH__

/* C++ std */
#include <string>
#include <vector>
#include <utility>

/* kscript C API */
#include <ks/ks.h>


using namespace std;

namespace ks {



/** Utility Functions **/

/* Return a kscript string from a UTF8 C++ stirng */
static ks_str make_str(const string& name) {
    return ks_str_new(name.size(), name.c_str());
}

/* Return a kscript module with C++ string initializers, and a vector of members 
 * NOTE: References are absorbed from 'members.second'
 */
static ks_module make_module(const string& name, const string& src_name, const string& doc, const vector< pair<string, kso> >& members) {
    ks_module res = ks_module_new(name.c_str(), src_name.c_str(), doc.c_str(), NULL);

    for (vector< pair<string, kso> >::const_iterator it = members.begin(); it != members.end(); ++it) {
        ks_dict_set_c(res->attr, (*it).first.c_str(), (*it).second);
    }

    return res;
}

/* Return a kscript type with C++ string initializers, and a vector of members 
 * NOTE: References are absorbed from 'members.second'
 */
static ks_type make_type(const string& name, ks_type base, int sz, int pos_attr, const string& doc, const vector< pair<string, kso> >& members) {
    ks_type res = ks_type_new(name.c_str(), base, sz, pos_attr, doc.c_str(), NULL);

    for (vector< pair<string, kso> >::const_iterator it = members.begin(); it != members.end(); ++it) {
        ks_type_set_c(res, it->first.c_str(), it->second);
    }

    return res;
}


/* Convert a C-style kscript string object to a C++ STL string */
static string get_string(ks_str str) {
    return string(str->data, str->len_b);
}



/* ks::Ref - wrapper around 'kso', the C-style kscript object
 *
 * This class represents an active reference to an object, so that
 *   the destructor automatically deconstructs it
 * 
 * 
 * ```
 * kso cobj = ...;
 * ks::Ref cxxobj = ks::Ref(cobj);
 * ```
 *
 * Or, if you want the object creation to create a new reference, run:
 * 
 * ```
 * kso cobj = ...;
 * ks::Ref cxxobj = ks::Ref(cobj, true);
 * ```
 * 
 * 
 */
struct Ref {
    kso ob;

    Ref(kso ob_, bool new_ref=false) : ob(ob_) {
        if (new_ref) KS_INCREF(ob);

    }

    ~Ref() {
        KS_DECREF(ob);
    }

    /* Increment reference count */
    void incref() {
        KS_INCREF(ob);
    }

    /* Decrement reference count */
    void decref() {
        KS_DECREF(ob);
    }

    /* Return a new reference to the object */
    Ref& newref() {
        KS_INCREF(ob);
        return *this;
    }

    string __str() {
        ks_str val = ks_fmt("%S", this);
        string res = get_string(val);
        KS_DECREF(val);
        return res;
    }

    string __repr() {
        ks_str val = ks_fmt("%R", this);
        string res = get_string(val);
        KS_DECREF(val);
        return res;
    }
};


};

#endif /* KSCXX_HH__ */
