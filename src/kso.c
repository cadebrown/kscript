/* kso.c - routines dealing with object management
 *
 * @author: Cade Brown <cade@kscript.org>
 */
#include <ks/impl.h>
#include <ks/compiler.h>


bool kso_issub(ks_type a, ks_type b) {
    if (a == b) return true;
    if (a == kst_object) return false;
    return kso_issub(a->i__base, b);
}
bool kso_isinst(kso a, ks_type b) {
    return kso_issub(a->type, b);
}


bool kso_inrepr(kso obj) {
    ksos_thread th = ksos_thread_get();

    int i = th->inrepr->len;
    while (--i >= 0) {
        if (obj == th->inrepr->elems[i]) return true;
    }

    /* Not found */
    ks_list_push(th->inrepr, obj);
    return false;
}


void kso_outrepr() {
    ksos_thread th = ksos_thread_get();

    ks_list_popu(th->inrepr);
}


bool kso_truthy(kso ob, bool* out) {
    if (kso_issub(ob->type, kst_int) && ob->type->i__bool == kst_int->i__bool) {
        ks_int obi = (ks_int)ob;
        #ifdef KS_INT_GMP
        *out = mpz_cmp_si(obi->val, 0) != 0;
        return true;
        #endif
    } else if (kso_issub(ob->type, kst_none) || kso_issub(ob->type, kst_undefined)) {
        *out = false;
        return true;
    } else if (kso_issub(ob->type, kst_float) && ob->type->i__bool == kst_float->i__bool) {
        ks_float obf = (ks_float)ob;
        *out = obf->val != 0;
        return true;
    } else if (kso_issub(ob->type, kst_complex) && ob->type->i__bool == kst_complex->i__bool) {
        ks_complex obf = (ks_complex)ob;
        *out = !KS_CC_EQRI(obf->val, 0, 0);
        return true;
    } else if (kso_issub(ob->type, kst_str) && ob->type->i__bool == kst_str->i__bool) {
        *out = ((ks_str)ob)->len_b > 0;
        return true;
    } else if (ob->type->i__bool) {
        kso bo = kso_call(ob->type->i__bool, 1, &ob);
        if (!bo) return NULL;

        bool res = kso_truthy(bo, out);
        KS_DECREF(bo);
        return res;
    }

    /*KS_THROW_EXC(ks_T_TypeError, "Could not convert '%T' object to 'bool'", obj);
    return false;*/
    *out = true;
    return true;
}

bool kso_cmp(kso L, kso R, int* out) {
    if (kso_isinst(L, kst_str) && kso_isinst(R, kst_str)) {
        *out = ks_str_cmp((ks_str)L, (ks_str)R);
        return true;
    } else if (kso_isinst(L, kst_int) && kso_isinst(R, kst_int) && L->type->i__cmp == kst_int->i__cmp) {
        *out = mpz_cmp(((ks_int)L)->val, ((ks_int)R)->val);
        return true;
    } else if (L->type->i__cmp == kst_object->i__cmp) {
        ks_uint aL = (ks_uint)L, aR = (ks_uint)R;
        *out = aL == aR ? 0 : (aL < aR ? -1 : +1);
        return true;
    } else if (L->type->i__cmp || R->type->i__cmp) {
        kso r = KSO_UNDEFINED;
        if (L->type->i__cmp) {
            r = kso_call(L->type->i__cmp, 2, (kso[]){ L, R });
            if (!r) return NULL;
        }

        if (r == KSO_UNDEFINED) {
            if (R->type->i__cmp) {
                r = kso_call(R->type->i__cmp, 2, (kso[]){ L, R });
                if (!r) return NULL;

                if (r == KSO_UNDEFINED) {

                } else {
                    ks_cint vv;
                    if (!kso_get_ci(r, &vv)) {
                        KS_DECREF(r);
                        *out = vv < 0 ? -1 : (vv > 0 ? 1 : 0);
                        return true;
                    }

                    *out = vv;
                    return true;
                }
            }

        } else {
            ks_cint vv;
            if (!kso_get_ci(r, &vv)) {
                KS_DECREF(r);
                        *out = vv < 0 ? -1 : (vv > 0 ? 1 : 0);
                return true;
            }

            *out = vv;
            return true;
        }
    }

    ks_uint aL = (ks_uint)L, aR = (ks_uint)R;
    *out = aL == aR ? 0 : (aL < aR ? -1 : +1);
    return true;
}


bool kso_eq(kso L, kso R, bool* out) {
    if (kso_isinst(L, kst_str) && kso_isinst(R, kst_str)) {
        *out = L == R || ks_str_eq((ks_str)L, (ks_str)R);
        return true;
    } else if (kso_isinst(L, kst_int) && kso_isinst(R, kst_int) && L->type->i__eq == kst_int->i__eq) {
        if (L == R) {
            *out = true;
        } else {
            *out = mpz_cmp(((ks_int)L)->val, ((ks_int)R)->val) == 0;
        }
        return true;
    } else if (L->type->i__eq == kst_object->i__eq) {
        *out = L == R;
        return true;
    } else if (L->type->i__eq || R->type->i__eq) {
        kso r = KSO_UNDEFINED;
        if (L->type->i__eq) {
            r = kso_call(L->type->i__eq, 2, (kso[]){ L, R });
            if (!r) return NULL;
        }

        if (r == KSO_UNDEFINED) {
            if (R->type->i__eq) {
                r = kso_call(R->type->i__eq, 2, (kso[]){ L, R });
                if (!r) return NULL;

                if (r == KSO_UNDEFINED) {

                } else {
                    bool res = kso_truthy(r, out);
                    KS_DECREF(r);
                    return res;
                }
            }

        } else {
            bool res = kso_truthy(r, out);
            KS_DECREF(r);
            return res;
        }
    }

    //assert(false);
    *out = L == R;
    return true;
}

bool kso_hash(kso ob, ks_hash_t* val) {
    if (kso_isinst(ob, kst_int) && ob->type->i__hash == kst_int->i__hash) {
        ks_int v = (ks_int)ob;

        /* Calculate hash */
        *val = mpz_fdiv_ui(v->val, KS_HASH_P);
        return true;
    } else if (kso_isinst(ob, kst_str) && ob->type->i__hash == kst_str->i__hash) {
        *val = ((ks_str)ob)->v_hash;
        return true;
    } else if (kso_isinst(ob, kst_tuple) && ob->type->i__hash == kst_tuple->i__hash) {
        *val = 0;
        ks_tuple v = (ks_tuple)ob;
        ks_cint i;
        ks_hash_t ho;
        for (i = 0; i < v->len; ++i) {
            if (!kso_hash(v->elems[i], &ho)) return false;
            *val = (*val) * KS_HASH_MUL + KS_HASH_ADD + ho;
        }
        return true;
    } else if (ob->type->i__hash == kst_object->i__hash) {
        *val = (ks_hash_t)ob;
        return true;
    } else if (ob->type->i__hash) {
        ks_int r = (ks_int)kso_call(ob->type->i__hash, 1, &ob);
        if (!r) return false;
        else if (!kso_issub(r->type, kst_int)) {
            KS_THROW(kst_TypeError, "'%T.__hash' returned non-int object of type '%T'", r);
            KS_DECREF(r);
            return false;
        }

        bool res = kso_hash((kso)r, val);
        KS_DECREF(r);
        return res;
    }


    KS_THROW(kst_TypeError, "'%T' object is not hashable", ob);
    return false;
}

bool kso_len(kso ob, ks_ssize_t* val) {
    assert(false);
}

bool kso_get_ci(kso ob, ks_cint* val) {
    if (kso_issub(ob->type, kst_int)) {
        ks_int obi = (ks_int)ob;
        #ifdef KS_INT_GMP
        if (mpz_fits_slong_p(obi->val)) {
            *val = mpz_get_si(obi->val);
            return true;
        } else {
            /* TODO: check for systems like Windows */
        }
        #endif

        KS_THROW(kst_OverflowError, "'%T' object was too large to convert to a C-style integer", ob);
        return false;
    } else if (ob->type->i__integral) {

        kso t = kso_call(ob->type->i__integral, 1, &ob);
        if (!t) return NULL;

        bool res = kso_get_ci(t, val);

        KS_DECREF(t);
        return res;
    }

    KS_THROW(kst_TypeError, "Failed to convert '%T' object to C-style integer", ob);
    return false;
}
bool kso_get_ui(kso ob, ks_uint* val) {
    if (kso_issub(ob->type, kst_int)) {
        ks_int obi = (ks_int)ob;
        #ifdef KS_INT_GMP
        if (mpz_fits_ulong_p(obi->val)) {
            *val = mpz_get_ui(obi->val);
            return true;
        } else {
            /* TODO: check for systems like Windows */
        }
        #endif

        KS_THROW(kst_OverflowError, "'%T' object was too large to convert to a C-style integer", ob);
        return false;
    }
    
    KS_THROW(kst_TypeError, "Failed to convert '%T' object to C-style integer", ob);
    return false;
}
bool kso_get_cf(kso ob, ks_cfloat* val) {
    if (kso_issub(ob->type, kst_int)) {
        ks_int obi = (ks_int)ob;
        #ifdef KS_INT_GMP
        *val = mpz_get_d(obi->val);
        return true;
        #endif
    } else if (kso_issub(ob->type, kst_float)) {
        *val = ((ks_float)ob)->val;
        return true;
    } else if (ob->type->i__float) {
        kso obf = kso_call(ob->type->i__float, 1, &ob);
        if (!obf) return false;

        bool res = kso_get_cf(obf, val);
        KS_DECREF(obf);
        return res;
    }
    
    KS_THROW(kst_TypeError, "Failed to convert '%T' object to C-style float", ob);
    return false;
}
bool kso_get_cc(kso ob, ks_ccomplex* val) {
    if (kso_issub(ob->type, kst_int)) {
        ks_int obi = (ks_int)ob;
        #ifdef KS_INT_GMP
        *val = KS_CC_MAKE(mpz_get_d(obi->val), 0);
        return true;
        #endif
    } else if (kso_issub(ob->type, kst_float)) {
        *val = KS_CC_MAKE(((ks_float)ob)->val, 0);
        return true;
    } else if (kso_issub(ob->type, kst_complex)) {
        *val = ((ks_complex)ob)->val;
        return true;
    } else if (ob->type->i__complex) {
        kso obf = kso_call(ob->type->i__complex, 1, &ob);
        if (!obf) return false;

        bool res = kso_get_cc(obf, val);
        KS_DECREF(obf);
        return res;
    } else if (ob->type->i__float) {
        kso obf = kso_call(ob->type->i__float, 1, &ob);
        if (!obf) return false;

        bool res = kso_get_cc(obf, val);
        KS_DECREF(obf);
        return res;
    }
    
    KS_THROW(kst_TypeError, "Failed to convert '%T' object to C-style complex float", ob);
    return false;
}


ks_str kso_str(kso ob) {
    if (kso_issub(ob->type, kst_str)) {
        KS_INCREF(ob);
        return (ks_str)ob;
    } else {
        return ks_fmt("%S", ob);
    }
}
ks_bytes kso_bytes(kso ob) {
    if (kso_issub(ob->type, kst_bytes)) {
        KS_INCREF(ob);
        return (ks_bytes)ob;
    } else if (ob->type->i__bytes) {
        ks_bytes r = (ks_bytes)kso_call(ob->type->i__bytes, 1, &ob);
        if (!r) return NULL;

        if (!kso_issub(r->type, kst_bytes)) {
            KS_THROW(kst_TypeError, "'%T.__bytes()' returned non-'bytes' object of type '%T'", ob, r);
            KS_DECREF(r);
            return NULL;
        }

        return r;
    }

    KS_THROW(kst_TypeError, "Failed to convert '%T' object to 'bytes'", ob);
    return NULL;
}
ks_str kso_repr(kso ob) {
    return ks_fmt("%R", ob);
}
ks_number kso_number(kso ob) {
    if (kso_issub(ob->type, kst_number)) {
        KS_INCREF(ob);
        return (ks_number)ob;
    }
    return (ks_number)kso_call((kso)kst_number, 1, &ob);
}
ks_int kso_int(kso ob) {
    if (kso_issub(ob->type, kst_int)) {
        KS_INCREF(ob);
        return (ks_int)ob;
    } else if (ob->type->i__int) {
        ks_int res = (ks_int)kso_call(ob->type->i__int, 1, &ob);
        if (!res) return NULL;
        if (!kso_issub(res->type, kst_int)) {
            KS_THROW(kst_TypeError, "'%T.__int()' returned non-'bytes' object of type '%T'", ob, res);
            KS_DECREF(res);
            return NULL;
        }
        return res;
    } else if (ob->type->i__integral) {
        ks_int res = (ks_int)kso_call(ob->type->i__integral, 1, &ob);
        if (!res) return NULL;
        if (!kso_issub(res->type, kst_int)) {
            KS_THROW(kst_TypeError, "'%T.__integral()' returned non-'bytes' object of type '%T'", ob, res);
            KS_DECREF(res);
            return NULL;
        }
        return res;
    }

    KS_THROW(kst_TypeError, "Failed to convert '%T' object to 'int'", ob);
    return NULL;
}


bool kso_is_num(kso obj) {
    return kso_issub(obj->type, kst_number);
}

bool kso_is_int(kso obj) {
    return kso_issub(obj->type, kst_int) || obj->type->i__integral;
}

bool kso_is_float(kso obj) {
    return kso_issub(obj->type, kst_float) || obj->type->i__float;
}

bool kso_is_complex(kso obj) {
    return kso_issub(obj->type, kst_complex) || obj->type->i__complex;
}
bool kso_is_iterable(kso obj) {
    return obj->type->i__iter || obj->type->i__next;
}

bool kso_is_callable(kso obj) {
    return kso_issub(obj->type, kst_type) || kso_issub(obj->type, kst_func) || kso_issub(obj->type, kst_partial) || obj->type->i__call;
}


ks_dict kso_try_getattr_dict(kso obj) {
    if (obj->type->ob_attr > 0) {
        return *(ks_dict*)((ks_uint)obj + obj->type->ob_attr);
    } else return NULL;
}

kso kso_getattr(kso ob, ks_str attr) {
    ksos_thread th = ksos_thread_get();

    if (kso_issub(ob->type, kst_type) && ob->type->i__getattr == kst_type->i__getattr) {

        kso res = ks_type_get((ks_type)ob, attr);
        if (res) {
            return res;
        } else if (kso_issub(th->exc->type, kst_AttrError)) {
            kso_catch_ignore();
        } else {
            return NULL;
        }

    } else if (ob->type->i__getattr != kst_object->i__getattr) {
        /* Attempt to resolve it */
        kso res = kso_call(ob->type->i__getattr, 2, (kso[]){ ob, (kso)attr });
        if (res) {
            return res;
        } else if (kso_issub(th->exc->type, kst_AttrError)) {
            kso_catch_ignore();
        } else {
            return NULL;
        }
    }

    ks_dict attrdict = kso_try_getattr_dict(ob);
    if (attrdict) {
        if (ks_str_eq_c(attr, "__attr", 6)) {
            return KS_NEWREF(attrdict);
        }

        /* Search for it */
        kso res = ks_dict_get_ih(attrdict, (kso)attr, attr->v_hash);
        if (res) {
            return res;
        }
    }

    /* Finally, search for a member function in the type's attribute */
    kso t_func = ks_type_get(ob->type, attr);
    if (t_func) {
        /* Wrap and return */
        ks_partial res = ks_partial_new(t_func, ob);
        KS_DECREF(t_func);
        return (kso)res;
    } else {
        kso_catch_ignore();
    }

    KS_THROW_ATTR(ob, attr);
    return NULL;
}

bool kso_setattr(kso ob, ks_str attr, kso val) {
    ksos_thread th = ksos_thread_get();

    if (ob->type->i__setattr != kst_object->i__setattr) {
        /* Attempt to resolve it */
        kso res = kso_call(ob->type->i__setattr, 3, (kso[]){ ob, (kso)attr, val});
        if (res) {
            KS_DECREF(res);
            return true;
        } else if (kso_issub(th->exc->type, kst_AttrError)) {
            kso_catch_ignore();
        } else {
            return false;
        }
    }

    ks_dict attrdict = kso_try_getattr_dict(ob);
    if (attrdict) {

        /* Search for it */
        if (ks_dict_set_h(attrdict, (kso)attr, attr->v_hash, val)) {
            return true;
        } else {
            kso_catch_ignore();
        }
    }

    KS_THROW_ATTR(ob, attr);
    return false;
}
bool kso_delattr(kso ob, ks_str attr);



kso kso_getelem(kso ob, kso key) {
    return kso_getelems(2, (kso[]){ ob, key });
}
bool kso_setelem(kso ob, kso key, kso val) {
    return kso_setelems(3, (kso[]){ ob, key, val });
}
bool kso_delelem(kso ob, kso key) {
    return kso_delelems(2, (kso[]){ ob, key });
}
kso kso_getelems(int n_keys, kso* keys) {
    assert(n_keys > 0);
    kso ob = keys[0];

    if (kso_issub(ob->type, kst_str) && ob->type->i__getelem == kst_str->i__getelem) {
        if (n_keys != 2) {
            KS_THROW(kst_ArgError, "Expected 2 arguments to element indexing operation");
            return NULL;
        }
        ks_str lob = (ks_str)ob;

        if (kso_issub(keys[1]->type, kst_slice)) {
            ks_cint first, last, delta;
            if (!ks_slice_get_citer((ks_slice)keys[1], lob->len_c, &first, &last, &delta)) return NULL;

            /* Check for specific cases */
            if (first == last) return (kso)ks_str_new(0, NULL);
            ks_cint len = (last - first) / delta;

            if (delta == 1 && KS_STR_IS_ASCII(lob)) {
                /* Direct substring */
                return (kso)ks_str_new(len, lob->data + first);
            }

            if (delta == 1) {
                /* Find range of bytes */
                int pb = 0, pc = 0;

                while (pc < first) {
                    pc++;
                    pb++;
                    while (KS_UCP_IS_CONT(lob->data[pb])) pb++;
                }

                int spb = pb;

                while (pc < last) {
                    pc++;
                    pb++;
                    while (KS_UCP_IS_CONT(lob->data[pb])) pb++;
                }

                return (kso)ks_str_new(pb - spb, lob->data + spb);

            } else {
                /* Now, build up a string */
                ksio_StringIO sio = ksio_StringIO_new();
                struct ks_str_citer cit = ks_str_citer_make(lob);

                /* Build the string */
                ks_str_citer_seek(&cit, first);
                ks_cint ct = first;

                do {
                    int lbyi = cit.cbyi;
                    ks_str_citer_next(&cit);
                    ksio_addbuf(sio, cit.cbyi - lbyi, lob->data + lbyi);
                    ks_str_citer_seek(&cit, cit.cchi + delta - 1);
                    ct += delta;
                } while (ct != last);

                return (kso)ksio_StringIO_getf(sio);
            }

        } else {
            ks_cint idx;
            if (!kso_get_ci(keys[1], &idx)) {
                return NULL;
            }
            if (idx < 0) idx += lob->len_c;
            if (idx < 0 || idx >= lob->len_c) {
                KS_THROW_INDEX(ob, keys[1]);
                return NULL;
            }
            if (KS_STR_IS_ASCII(lob)) {
                return (kso)ks_str_new(1, lob->data + idx);
            } else {
                /* Seek to position */
                int c = 0, b = 0, lb = 0;
                while (c < idx) {
                    /* Parse single character */
                    lb = b;
                    do {
                        b++;
                    } while (b < lob->len_b && (((unsigned char*)lob->data)[b] & 0x80) != 0);
                    c++;
                }

                return (kso)ks_str_new(b - lb, lob->data + lb);
            }
        }
    } else if (kso_issub(ob->type, kst_bytes) && ob->type->i__getelem == kst_bytes->i__getelem) {
        if (n_keys != 2) {
            KS_THROW(kst_ArgError, "Expected 2 arguments to element indexing operation");
            return NULL;
        }
        ks_bytes lob = (ks_bytes)ob;

        ks_cint idx;
        if (!kso_get_ci(keys[1], &idx)) {
            return NULL;
        }
        if (idx < 0) idx += lob->len_b;
        if (idx < 0 || idx >= lob->len_b) {
            KS_THROW_INDEX(ob, keys[1]);
            return NULL;
        }
        return (kso)ks_bytes_new(1, lob->data + idx);

    } else if (kso_issub(ob->type, kst_list) && ob->type->i__getelem == kst_list->i__getelem) {
        if (n_keys != 2) {
            KS_THROW(kst_ArgError, "Expected 2 arguments to element indexing operation");
            return NULL;
        }
        ks_list lob = (ks_list)ob;

        if (kso_issub(keys[1]->type, kst_slice)) {
            ks_cint first, last, delta;
            if (!ks_slice_get_citer((ks_slice)keys[1], lob->len, &first, &last, &delta)) {
                return NULL;
            }
            ks_list res = ks_list_new(0, NULL);
            ks_cint i;
            for (i = first; i != last; i += delta) {
                ks_list_push(res, lob->elems[i]);
            }

            return (kso)res;
        } else {
            ks_cint idx;
            if (!kso_get_ci(keys[1], &idx)) {
                return NULL;
            }
            if (idx < 0) idx += lob->len;
            if (idx < 0 || idx >= lob->len) {
                KS_THROW_INDEX(ob, keys[1]);
                return NULL;
            }

            return KS_NEWREF(lob->elems[idx]);
        }
    } else if (kso_issub(ob->type, kst_tuple) && ob->type->i__getelem == kst_tuple->i__getelem) {
        if (n_keys != 2) {
            KS_THROW(kst_ArgError, "Expected 2 arguments to element indexing operation");
            return NULL;
        }
        ks_tuple lob = (ks_tuple)ob;

        if (kso_issub(keys[1]->type, kst_slice)) {
            ks_cint first, last, delta;
            if (!ks_slice_get_citer((ks_slice)keys[1], lob->len, &first, &last, &delta)) {
                return NULL;
            }
            ks_list res = ks_list_new(0, NULL);
            ks_cint i;
            for (i = first; i != last; i += delta) {
                ks_list_push(res, lob->elems[i]);
            }
            ks_tuple rr = ks_tuple_new(res->len, res->elems);
            KS_DECREF(res);
            return (kso)rr;
        } else {
            ks_cint idx;
            if (!kso_get_ci(keys[1], &idx)) {
                return NULL;
            }
            if (idx < 0) idx += lob->len;
            if (idx < 0 || idx >= lob->len) {
                KS_THROW_INDEX(ob, keys[1]);
                return NULL;
            }

            return KS_NEWREF(lob->elems[idx]);
        }
    } else if (kso_issub(ob->type, kst_dict) && ob->type->i__getelem == kst_dict->i__getelem) {
        if (n_keys != 2) {
            KS_THROW(kst_ArgError, "Expected 2 arguments to element indexing operation");
            return NULL;
        }
        return ks_dict_get((ks_dict)ob, keys[1]);

    } else if (ob->type->i__getelem) {
        return kso_call(ob->type->i__getelem, n_keys, keys);
    }

    KS_THROW(kst_TypeError, "'%T' object did not support element indexing", ob);
    return NULL;
}
bool kso_setelems(int n_keys, kso* keys) {
    assert(n_keys > 1);
    kso ob = keys[0];
    if (kso_issub(ob->type, kst_list) && ob->type->i__setelem == kst_list->i__setelem) {
        if (n_keys != 3) {
            KS_THROW(kst_ArgError, "Expected 3 arguments to element indexing operation");
            return NULL;
        }
        ks_list lob = (ks_list)ob;

        ks_cint idx;
        if (!kso_get_ci(keys[1], &idx)) {
            return NULL;
        }
        if (idx < 0) idx += lob->len;
        if (idx < 0 || idx >= lob->len) {
            KS_THROW_INDEX(ob, keys[1]);
            return NULL;
        }
        
        KS_INCREF(keys[2]);
        KS_DECREF(lob->elems[idx]);
        lob->elems[idx] = keys[2];

        return true;
    } else if (kso_issub(ob->type, kst_dict) && ob->type->i__setelem == kst_dict->i__setelem) {
        if (n_keys != 3) {
            KS_THROW(kst_ArgError, "Expected 3 arguments to element indexing operation");
            return NULL;
        }
        return ks_dict_set((ks_dict)ob, keys[1], keys[2]);

    } else if (ob->type->i__setelem) {
        kso rr = kso_call(ob->type->i__setelem, n_keys, keys);
        if (!rr) return false;
        KS_DECREF(rr);
        return true;
    }

    KS_THROW(kst_TypeError, "'%T' object did not support element indexing assignment", ob);
    return NULL;
}
bool kso_delelems(int n_keys, kso* keys) {
    assert(n_keys > 0);
    assert(false);
}


kso kso_call_ext(kso func, int nargs, kso* args, ks_dict locals, ksos_frame closure) {
    ksos_thread th = ksos_thread_get();
    assert(th != NULL);
    int i, j, k;

    if (func == (kso)kst_type) {
        if (nargs == 1) {
            return KS_NEWREF(args[0]->type);
        } else {
            KS_THROW(kst_Error, "Calling 'type()' should take only 1 argument");
            return NULL;
        }
    }

    /* If '...' is called, recurse on the last function called */
    if (func == KSO_DOTDOTDOT) {
        if (th->frames->len < 1) {
            KS_THROW(kst_Error, "Calling '...' has to happen inside of another frame (but there were none)");
            return NULL;
        } else {
            ksos_frame parframe = (ksos_frame)th->frames->elems[th->frames->len - 1];
            return kso_call(parframe->func, nargs, args);
        }
    }
    

    kso res = NULL;
    if (kso_issub(func->type, kst_func) && func->type->i__call == kst_func->i__call) {
        /* If given a standard function which is not a subtype that overrides the calling feature */
        ksos_frame frame = ksos_frame_new(func);
        ks_list_push(th->frames, (kso)frame);

        ks_func f = (ks_func)func;
        if (f->is_cfunc) {
            /* Execute the C-style function directly */
            res = f->cfunc(nargs, args);
        } else {
            /* Execute a bytecode directly */

            /* Bytecode function */
            if (!frame->locals) frame->locals = ks_dict_new(NULL);
            int i;

            if (!frame->closure) {
                if (f->bfunc.closure) {
                    KS_INCREF(f->bfunc.closure);
                    frame->closure = (ksos_frame)f->bfunc.closure;
                }
            }

            if (f->bfunc.vararg_idx >= 0) {
                /* Vararg calling */
                if (nargs < f->bfunc.n_req) {
                    KS_THROW(kst_ArgError, "Expected at least %i arguments, but got %i", f->bfunc.n_req, nargs);
                } else {
                    int n_before = f->bfunc.vararg_idx;
                    int n_after = f->bfunc.n_pars - f->bfunc.vararg_idx - 1;
                    int n_va = nargs - (n_before + n_after);

                    for (i = 0; i < n_before; ++i) {
                        bool b = ks_dict_set_h(frame->locals, (kso)f->bfunc.pars[i].name, f->bfunc.pars[i].name->v_hash, args[i]);
                        assert(b);
                    }
                    ks_list vas = ks_list_new(n_va, args + i);
                    ks_dict_set_h(frame->locals, (kso)f->bfunc.pars[i].name, f->bfunc.pars[i].name->v_hash, (kso)vas);
                    i += n_va;
                    KS_DECREF(vas);

                    int j;
                    for (j = n_before+1; i < nargs; ++i, ++j) {
                        bool b = ks_dict_set_h(frame->locals, (kso)f->bfunc.pars[j].name, f->bfunc.pars[j].name->v_hash, args[i]);
                        assert(b);
                    }

                    res = _ks_exec((ks_code)f->bfunc.bc, NULL);
                }
            } else {
                /* Standard calling */
                if (f->bfunc.n_req == f->bfunc.n_pars && nargs != f->bfunc.n_req) {
                    KS_THROW(kst_ArgError, "Expected %i arguments, but got %i", f->bfunc.n_req, nargs);
                } else if (nargs < f->bfunc.n_req || nargs > f->bfunc.n_pars) {
                    KS_THROW(kst_ArgError, "Expected between %i and %i arguments, but got %i", f->bfunc.n_req, f->bfunc.n_pars, nargs);
                } else {
                    for (i = 0; i < f->bfunc.n_pars; ++i) {
                        bool b = ks_dict_set_h(frame->locals, (kso)f->bfunc.pars[i].name, f->bfunc.pars[i].name->v_hash, i < nargs ? args[i] : f->bfunc.pars[i].defa);
                        assert(b);
                    }

                    res = _ks_exec((ks_code)f->bfunc.bc, NULL);
                }
            }
        }

        ks_list_popu(th->frames);
        KS_DECREF(frame);

    } else if (kso_issub(func->type, kst_code) && func->type->i__call == kst_code->i__call) {
        /* Calling a bytecode should just execute it */
        ksos_frame frame = ksos_frame_new(func);
        ks_list_push(th->frames, (kso)frame);
        if (closure) KS_INCREF(closure);
        frame->closure = closure;

        if (locals) {
            KS_INCREF(locals);
            frame->locals = locals;
        } else {
            /* TODO: possibly don't use dictionaries until they're needed? */
            frame->locals = ks_dict_new(NULL);
        }
        ks_type _in = NULL;
        if (nargs >= 1 && kso_issub(args[0]->type, kst_type)) _in = (ks_type)args[0];
        res = _ks_exec((ks_code)func, _in);

        ks_list_popu(th->frames);
        KS_DECREF(frame);
    } else if (kso_issub(func->type, kst_partial) && func->type->i__call == kst_partial->i__call) {
        /* Fill in arguments to partial function */
        ks_partial f = (ks_partial)func;

        /* Create a new buffer with all the arguments */
        int new_nargs = nargs + f->n_args;
        kso* new_args = ks_zmalloc(sizeof(*new_args), new_nargs);

        for (i = 0, j = 0, k = 0; i < new_nargs; ++i) {
            if (j < f->n_args && f->args[j].idx == i) {
                /* Take the next from the pre-filled-in ones */
                new_args[i] = f->args[j++].val;
            } else {
                /* Take it from params */
                new_args[i] = args[k++];
            }
        }
        assert(i == new_nargs && j == f->n_args && k == nargs);

        /* Call the thing being wrapped */
        res = kso_call(f->of, new_nargs, new_args);

        ks_free(new_args);

    } else if (kso_issub(func->type, kst_type) && func->type->i__call == kst_type->i__call) {
        /* We have a type that has not overriden '__call', so treat it like a constructor */
        /* Don't push a stack frame on for this */
        ks_type tp = (ks_type)func;

        /* Initialization scheme:
         *
         * By default, when a type doesn't override '__call', it means calling it should act
         *   as a constructor for the type. It does this with the following steps:
         * 
         * # Allocates a new instance (default: 'KSO_NEW()', and '*args' are ignored)
         * tmp = tp.__new(tp, *args)
         * # Initializes the instance (default: none)
         * tp.__init(tmp, *args)
         * 
         * Note that '*args' are passed to the new and init functions (which may ignore them)
         * 
         */
        if (tp->i__new == kst_type->i__new) {
            /* Default allocation */
            res = KSO_NEW(kso, tp);
        } else {
            /* Actually call allocator */
            int new_nargs = nargs + 1;
            kso* new_args = ks_zmalloc(sizeof(*new_args), new_nargs);
            new_args[0] = (kso)tp;
            for (i = 0; i < nargs; ++i) new_args[i + 1] = args[i];
            res = kso_call(tp->i__new, new_nargs, new_args);
            ks_free(new_args);
        }

        if (res) {
            /* Successfully allocated it */
            if (tp->i__init == kst_type->i__init) {
                /* Default initializer, which is nothing */

            } else {
                /* Custom initializer */
                int new_nargs = nargs + 1;
                kso* new_args = ks_zmalloc(sizeof(*new_args), new_nargs);
                new_args[0] = (kso)res;
                for (i = 0; i < nargs; ++i) new_args[i + 1] = args[i];
                kso t = kso_call(tp->i__init, new_nargs, new_args);
                ks_free(new_args);
                if (!t) {
                    /* Failed to initialize */
                    KS_DECREF(res);
                    res = NULL;
                } else {
                    KS_DECREF(t);
                }
            }
        }
        
    } else if (func->type->i__call) {
        ksos_frame frame = ksos_frame_new(func);
        ks_list_push(th->frames, (kso)frame);

        /* Allocate new arguments with the instance at the front */
        int new_nargs = nargs + 1;
        kso* new_args = ks_zmalloc(sizeof(*new_args), new_nargs);
        new_args[0] = (kso)func;
        for (i = 0; i < nargs; ++i) new_args[i + 1] = args[i];

        res = kso_call(func->type->i__call, new_nargs, new_args);

        ks_free(new_args);

        ks_list_popu(th->frames);
        KS_DECREF(frame);
    } else {
        KS_THROW(kst_TypeError, "'%T' object was not callable", func);
    }

    return res;
}

kso kso_eval(ks_str src, ks_str fname, ks_dict locals) {
    /* Turn the input code into a list of tokens */
    ks_tok* toks = NULL;
    ks_ssize_t n_toks = ks_lex(fname, src, &toks);
    if (n_toks < 0) {
        ks_free(toks);
        return NULL;
    }

    /* Parse the tokens into an AST */
    ks_ast prog = ks_parse_prog(fname, src, n_toks, toks);
    ks_free(toks);
    if (!prog) {
        return NULL;
    }

    while (prog->kind == KS_AST_BLOCK && prog->args->len == 1) {
        ks_ast ch = (ks_ast)prog->args->elems[0];
        KS_INCREF(ch);
        KS_DECREF(prog);
        prog = ch;
    }

    /* Check if we should print */
    bool do_print = false;
    if (ks_ast_is_expr(prog->kind)) {
        do_print = true;
        prog = ks_ast_newn(KS_AST_RET, 1, &prog, NULL, prog->tok);
    }

    /* Compile the AST into a bytecode object which can be executed */
    ks_code code = ks_compile(fname, src, prog, NULL);
    KS_DECREF(prog);
    if (!code) {
        return NULL;
    }

    /* Execute the program, which should return the value */
    kso res = kso_call_ext((kso)code, 0, NULL, locals, NULL);
    KS_DECREF(code);
    if (!res) return NULL;

    return res;
}

kso kso_call(kso func, int nargs, kso* args) {
    return kso_call_ext(func, nargs, args, NULL, NULL);
}


kso kso_iter(kso ob) {
    if (ob->type->i__iter) {
        return kso_call(ob->type->i__iter, 1, &ob);
    } else if (ob->type->i__next) {
        /* Already is iterable */
        return KS_NEWREF(ob);
    }

    KS_THROW(kst_Error, "'%T' object was not iterable", ob);
    return NULL;
}

kso kso_next(kso ob) {
    if (kso_issub(ob->type, kst_str_iter) && ob->type->i__next == kst_str_iter->i__next) {
        /* String iteration */
        ks_str_iter it = (ks_str_iter)ob;
        if (it->pos >= it->of->len_b) {
            KS_OUTOFITER();
            return NULL;
        }

        /* Otherwise, continue and skip UTF-8 continuations */
        int ct = 0;
        do {
            ct++;
        } while (ct < 4 && (((unsigned char*)it->of->data)[it->pos + ct] & 0x80) != 0);
        assert(ct > 0);
        ks_str res = ks_str_new(ct, it->of->data + it->pos);
        it->pos += ct;
        return (kso)res;
    } else if (kso_issub(ob->type, kst_list_iter) && ob->type->i__next == kst_list_iter->i__next) {
        ks_list_iter it = (ks_list_iter)ob;
        if (it->pos >= it->of->len) {
            KS_OUTOFITER();
            return NULL;
        }

        return KS_NEWREF(it->of->elems[it->pos++]);
    } else if (kso_issub(ob->type, kst_tuple_iter) && ob->type->i__next == kst_tuple_iter->i__next) {
        ks_tuple_iter it = (ks_tuple_iter)ob;
        if (it->pos >= it->of->len) {
            KS_OUTOFITER();
            return NULL;
        }

        return KS_NEWREF(it->of->elems[it->pos++]);
    } else if (kso_issub(ob->type, kst_set_iter) && ob->type->i__next == kst_set_iter->i__next) {
        ks_set_iter it = (ks_set_iter)ob;
        if (it->pos >= it->of->len_ents) {
            KS_OUTOFITER();
            return NULL;
        }
        while (!it->of->ents[it->pos].key) it->pos++;
        if (it->pos >= it->of->len_ents) {
            KS_OUTOFITER();
            return NULL;
        }
        return KS_NEWREF(it->of->ents[it->pos++].key);
    } else if (kso_issub(ob->type, kst_dict_iter) && ob->type->i__next == kst_dict_iter->i__next) {
        ks_dict_iter it = (ks_dict_iter)ob;
        if (it->pos >= it->of->len_ents) {
            KS_OUTOFITER();
            return NULL;
        }
        while (!it->of->ents[it->pos].key) it->pos++;
        if (it->pos >= it->of->len_ents) {
            KS_OUTOFITER();
            return NULL;
        }
        return KS_NEWREF(it->of->ents[it->pos++].key);
    } else if (kso_issub(ob->type, kst_range_iter) && ob->type->i__next == kst_range_iter->i__next) {
        /* Range iterator */
        ks_range_iter it = (ks_range_iter)ob;
        if (it->done) {
            KS_OUTOFITER();
            return NULL;
        }

        /* Shortcut for speedup */
        if (it->use_ci) {
            ks_cint n_cur = it->_ci.cur + it->_ci.step;
            int cmp_ce = (it->_ci.cur > it->_ci.end) - (it->_ci.cur < it->_ci.end);

            if (cmp_ce == 0 || (it->cmp_step_0 > 0 && cmp_ce > 0) || (it->cmp_step_0 < 0 && cmp_ce < 0)) {
                it->done = true;
                KS_OUTOFITER();
                return NULL;
            }

            ks_int res = ks_int_new(it->_ci.cur);
            it->_ci.cur = n_cur;

            return (kso)res;
        }

        /* Determine the next value */
        if (it->cur) {

            ks_int newcur = (ks_int)ks_bop_add((kso)it->cur, (kso)it->of->step);
            if (!newcur) return NULL;
            assert(newcur->type == kst_int);
            KS_DECREF(it->cur);
            it->cur = (ks_int)newcur;

        } else {
            it->cur = (ks_int)KS_NEWREF(it->of->start);
        }

        /* Do bounds check */
        int cmp_ce = ks_int_cmp(it->cur, it->of->end);

        /* Do check with step direction */
        if (cmp_ce == 0 || (it->cmp_step_0 > 0 && cmp_ce > 0) || (it->cmp_step_0 < 0 && cmp_ce < 0)) {
            it->done = true;
            KS_OUTOFITER();
            return NULL;
        }

        return KS_NEWREF(it->cur);

    } else if (ob->type->i__next) {
        return kso_call(ob->type->i__next, 1, &ob);
    } else {
        /* Default to 'next(iter(ob))' */
        kso it = kso_iter(ob);
        if (!it) return NULL;
        kso res = kso_next(it);
        KS_DECREF(it);
        return res;
    }
}

void* kso_throw(ks_Exception exc) {
    if (!kso_issub(exc->type, kst_Exception)) {
        KS_THROW(kst_Exception, "Tried to throw '%T' object. Only subtypes of 'Exception' may be thrown", exc);
        return NULL;
    }

    ksos_thread th = ksos_thread_get();
    assert(th != NULL);

    /* Set the inner exception to the current exception thrown (which may be NULL) */
    exc->inner = th->exc;

    /* Set the thread's exception */
    th->exc = exc;

    ks_list_clear(exc->frames);
    ks_size_t i;
    for (i = 0; i < th->frames->len; ++i) {
        ksos_frame nf = ksos_frame_copy((ksos_frame)th->frames->elems[i]);
        ks_list_pushu(exc->frames, (kso)nf);
    }



    return NULL;
}

void* kso_throw_c(ks_type tp, const char* cfile, const char* cfunc, int cline, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ks_Exception exc = ks_Exception_new_cv(tp, cfile, cfunc, cline, fmt, ap);
    va_end(ap);
    return kso_throw(exc);
}

ks_Exception kso_catch() {
    ksos_thread th = ksos_thread_get();

    ks_Exception res = th->exc;
    if (th->exc) th->exc = NULL;

    return res;
}

bool kso_catch_ignore() {
    ks_Exception exc = kso_catch();
    if (exc) {
        KS_DECREF(exc);
        return true;
    }
    return false;
}


bool kso_catch_ignore_print() {
    ks_Exception exc = kso_catch();
    if (exc) {
        ksio_add((ksio_BaseIO)ksos_stderr, KS_COL_RED KS_COL_BOLD "%T" KS_COL_RESET ": %S\n", exc, exc->what);
        ks_list frames = exc->frames;
        assert(frames != NULL);

        ksio_add((ksio_BaseIO)ksos_stderr, "Call Stack:\n");

        #define _PFRAME(_i) do { \
            ksos_frame frame = (ksos_frame)frames->elems[_i]; \
            ks_str tb = ksos_frame_get_tb(frame); \
            ksio_add((ksio_BaseIO)ksos_stderr, "  #%i: %S\n", _i, tb); \
            KS_DECREF(tb); \
        } while (0)

        int mxn = 10, i;
        if (frames->len > mxn) {
            for (i = 0; i < mxn/2; ++i) _PFRAME(i);
            ksio_add((ksio_BaseIO)ksos_stderr, "  ... (%i more)\n", (int)(frames->len - mxn));
            for (i = frames->len-mxn/2; i < frames->len; ++i) _PFRAME(i);
        } else {
            for (i = 0; i < frames->len; ++i) _PFRAME(i);
        }

        ksio_add((ksio_BaseIO)ksos_stderr, "In %R\n", ksos_thread_get());

        KS_DECREF(exc);

        return true;
    }
    return false;
}

void kso_exit_if_err() {
    if (kso_catch_ignore_print()) {
        exit(1);
    }
}



/** Internal methods **/

kso _kso_new(ks_type tp) {
    assert(tp->ob_sz > 0);
    kso res = ks_zmalloc(1, tp->ob_sz);
    memset(res, 0, tp->ob_sz);

    KS_INCREF(tp);
    res->type = tp;
    res->refs = 1;

    tp->num_obs_new++;

    if (tp->ob_attr > 0) {
        /* Initialize attribute dictionary */
        ks_dict* attr = (ks_dict*)(((ks_uint)res + tp->ob_attr));
        *attr = ks_dict_new(NULL);
    }

    return res;
}

void _kso_del(kso ob) {
    if (ob->refs < 0) {
        printf("BAD REFS ON: '%s' obj: %lli\n", ob->type->i__fullname->data, (long long int)ob->refs);
        exit(1);
    }

    if (ob->type->ob_attr > 0) {
        /* Initialize attribute dictionary */
        ks_dict* attr = (ks_dict*)(((ks_uint)ob + ob->type->ob_attr));
    
        KS_DECREF(*attr);
    }

    ob->type->num_obs_del++;
    KS_DECREF(ob->type);

    ks_free(ob);
}

kso _ks_newref(kso ob) {
    KS_INCREF(ob);
    return ob;
}

void _kso_free(kso obj, const char* file, const char* func, int line) {
    /*
    if (obj->refs != 0) {
        fprintf(stderr, "[ks] Trying to free <'%s' obj @ %p>, which had %i refs\n", obj->type->i__fullname__->chr, obj, (int)obj->refs);
    }
    */
    kso i__free = obj->type->i__free;

    if (i__free) {
        kso res = NULL;
        /* Call deconstructor */
        if (i__free->type == kst_func && ((ks_func)i__free)->is_cfunc) {
            /* At some point we have to call without creating a stack frame, since that
             *   would create infinite recursion
             */
            res = ((ks_func)i__free)->cfunc(1, &obj);
        } else {

            res = kso_call(i__free, 1, &obj);
        }

        if (!res) {
            fprintf(stderr, "Failed to free object @ %p\n", obj);
            kso_exit_if_err();
        }

    } else {
        KSO_DEL(obj);
    }
}

