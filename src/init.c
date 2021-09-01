/* init.c - kscript initialization routines
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#include <ks/impl.h>


void ks_init() {
    static bool has_init = false;
    if (has_init) return;
    has_init = true;

    /* Some initialization is needed for the objects created within types */
    kst_func->ob_size = sizeof(struct ks_func_s);
    kst_func->ob_attr = offsetof(struct ks_func_s, attr);

    /* Now, initialize all the standard types */

    ksi_object(); 
    ksi_type(); 
    ksi_exc();
    ksi_logger();

    ksi_none();
    ksi_undefined();
    ksi_dotdotdot();

    ksi_number();
      ksi_int();
        ksi_enum();
          ksi_bool();
      ksi_float();
      ksi_complex();

    ksi_str();
    ksi_bytes();
    ksi_regex();
    ksi_range();
    ksi_slice();

    ksi_tuple();
    ksi_list();
    ksi_set();
    ksi_dict();

    ksi_map();
    ksi_filter();
    ksi_zip();
    ksi_batch();

    ksi_queue();
    ksi_bst();
    ksi_graph();

    ksi_ast();
    ksi_code();

    /* Now, initialize all the standard functions */


    /* Now, initialize all the standard modules */

    ksi_os();

    /* Now, set up the global dictionary */

}

