/* ks/kpm.h - header for the 'kpm' (kscript package manager) module in kscript
 *
 * This is the default package manager, and installer
 * 
 * 
 * Functions are prefixed with 'kpm_'
 * 
 * @author:    Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KSKPM_H__
#define KSKPM_H__

#ifndef KS_H__
#include <ks/ks.h>
#endif



/* Returns the installation path for kpm modules
 */
KS_API ks_str kpm_get_path();

/* Sets the installation path for kpm modules
 */
KS_API void kpm_set_path(ks_str path);



/** kpm.cext module **/

/* kpm.cext.Project - C/C++ style extension project
 * 
 * Projects can be built with makefile-style rules
 */
typedef struct kpm_cext_project_s {
    KSO_BASE

    /* Generic attribute dictionary */
    ks_dict attr;

}* kpm_cext_project;



KS_API_DATA ks_type
    kpm_cextt_project

;


/* 'kpm' module */
KS_API_DATA ks_module
    ksm_kpm
;


#endif /* KSKPM_H__ */
