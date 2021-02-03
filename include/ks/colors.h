/* ks/colors.h - defines kscript colors, which are typically terminal escape codes
 * 
 * These colors are based partially on the list here: 
 * https://stackoverflow.com/questions/4842424/list-of-ansi-color-escape-sequences
 *
 * @author: Cade Brown <cade@kscript.org>
 */

#pragma once
#ifndef KS_COLORS_H__
#define KS_COLORS_H__

#if defined(KS_USE_COLORS)

#define KS_COL_BK_BLK "\x1b[40m"
#define KS_COL_BK_BLU "\x1b[44m"
#define KS_COL_BK_CYN "\x1b[46m"
#define KS_COL_BK_DFT "\x1b[49m"
#define KS_COL_BK_GRN "\x1b[42m"
#define KS_COL_BK_GRY "\x1b[100m"
#define KS_COL_BK_LBLU "\x1b[104m"
#define KS_COL_BK_LCYN "\x1b[106m"
#define KS_COL_BK_LGRN "\x1b[102m"
#define KS_COL_BK_LGRY "\x1b[47m"
#define KS_COL_BK_LMGA "\x1b[105m"
#define KS_COL_BK_LRED "\x1b[101m"
#define KS_COL_BK_LYLW "\x1b[103m"
#define KS_COL_BK_MGA "\x1b[45m"
#define KS_COL_BK_RED "\x1b[41m"
#define KS_COL_BK_WHT "\x1b[107m"
#define KS_COL_BK_YLW "\x1b[43m"
#define KS_COL_BLINK "\x1b[5m"
#define KS_COL_BLK "\x1b[30m"
#define KS_COL_BLU "\x1b[34m"
#define KS_COL_BOLD "\x1b[1m"
#define KS_COL_CYN "\x1b[36m"
#define KS_COL_DFT "\x1b[39m"
#define KS_COL_DIM "\x1b[2m"
#define KS_COL_FAIL "\x1b[31m"
#define KS_COL_GRN "\x1b[32m"
#define KS_COL_GRY "\x1b[90m"
#define KS_COL_HIDE "\x1b[8m"
#define KS_COL_LBLU "\x1b[94m"
#define KS_COL_LCYN "\x1b[96m"
#define KS_COL_LGRN "\x1b[92m"
#define KS_COL_LGRY "\x1b[37m"
#define KS_COL_LMGA "\x1b[95m"
#define KS_COL_LRED "\x1b[91m"
#define KS_COL_LYLW "\x1b[93m"
#define KS_COL_MGA "\x1b[35m"
#define KS_COL_RED "\x1b[31m"
#define KS_COL_RESET "\x1b[0m"
#define KS_COL_RVRS "\x1b[7m"
#define KS_COL_SUCC "\x1b[32m"
#define KS_COL_ULIN "\x1b[4m"
#define KS_COL_WARN "\x1b[33m"
#define KS_COL_WHT "\x1b[97m"
#define KS_COL_YLW "\x1b[33m"

#else

#define KS_COL_BK_BLK ""
#define KS_COL_BK_BLU ""
#define KS_COL_BK_CYN ""
#define KS_COL_BK_DFT ""
#define KS_COL_BK_GRN ""
#define KS_COL_BK_GRY ""
#define KS_COL_BK_LBLU ""
#define KS_COL_BK_LCYN ""
#define KS_COL_BK_LGRN ""
#define KS_COL_BK_LGRY ""
#define KS_COL_BK_LMGA ""
#define KS_COL_BK_LRED ""
#define KS_COL_BK_LYLW ""
#define KS_COL_BK_MGA ""
#define KS_COL_BK_RED ""
#define KS_COL_BK_WHT ""
#define KS_COL_BK_YLW ""
#define KS_COL_BLINK ""
#define KS_COL_BLK ""
#define KS_COL_BLU ""
#define KS_COL_BOLD ""
#define KS_COL_CYN ""
#define KS_COL_DFT ""
#define KS_COL_DIM ""
#define KS_COL_FAIL ""
#define KS_COL_GRN ""
#define KS_COL_GRY ""
#define KS_COL_HIDE ""
#define KS_COL_LBLU ""
#define KS_COL_LCYN ""
#define KS_COL_LGRN ""
#define KS_COL_LGRY ""
#define KS_COL_LMGA ""
#define KS_COL_LRED ""
#define KS_COL_LYLW ""
#define KS_COL_MGA ""
#define KS_COL_RED ""
#define KS_COL_RESET ""
#define KS_COL_RVRS ""
#define KS_COL_SUCC ""
#define KS_COL_ULIN ""
#define KS_COL_WARN ""
#define KS_COL_WHT ""
#define KS_COL_YLW ""

#endif

#endif /* KS_COLORS_H__ */
