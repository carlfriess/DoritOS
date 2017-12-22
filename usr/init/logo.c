//
//  logo.c
//  DoritOS
//
//  Created by Carl Friess on 22/12/2017.
//  Copyright © 2017 Carl Friess. All rights reserved.
//

#include <aos/syscalls.h>

void print_logo(void);

void print_logo(void) {

    sys_print("\n                `                                                                           \n", 94);
    sys_print("                `` -                                                                        \n", 93);
    sys_print("                   ```                                                                      \n", 93);
    sys_print("                   `.`                                                                      \n", 93);
    sys_print("                    .:/.`                                                                   \n", 93);
    sys_print("                    `-sNho:.`                                                               \n", 93);
    sys_print("                      -ymydhs/-``                                                           \n", 93);
    sys_print("                       -s/--:+oo+:.`                                                        \n", 93);
    sys_print("                        -+-   `.-::/:-.`                                                    \n", 93);
    sys_print("                         -:`       ``.---..`                                                \n", 93);
    sys_print("                         `--             `..-.``                                            \n", 93);
    sys_print("     `                    `-`                 `````                         ``              \n", 93);
    sys_print("   ```  ``.........``` `` ``.                   ```   ```       ``.-:::-```` ``.:/+++++/-`` \n", 93);
    sys_print("   ```  `:MMMMMMMMMNho.`` ``````                `+o/.`./s+-`` `.+dMMMMMMNh:```sNMMMMMMMd``  \n", 93);
    sys_print("   ```  `:MMMdhhhhmMMMNs.` .+:s:::-.``    ``-///:oyoo.:yMMMo/-`dMMmo++oyMMM+`oMMMNmmmmd.`   \n", 93);
    sys_print("   ```  `:MMNo.````-oMMMy``:hMMMMMMMd+.``.sNMMMMy/mMN:mMMMMMMs:MMN-    `sMMm`sMMMNs:````    \n", 93);
    sys_print("   ```  `:MMNo.     .sMMN./MMNyooosmMMd..sMMNo++:oMMN:/yMMNs/:/MMN. ````oMMm`.dMMMMMNy/``   \n", 93);
    sys_print("   ```  `:MMNo.     `sMMN.dMMs`````.MMN/`dMMN.` .oMMN:.oMMN:../MMN. `-odo:sd```/hNMMMMMm-`  \n", 93);
    sys_print("   ```  `:MMNo.     `yMMN`mMMs. . `.NMN+`dMMN.` `oMMN:.oMMN:``/MMh:-oys:/dMm````..:oNMMMy`` \n", 93);
    sys_print("   ```  `:MMNo````./yMMNs`dMMy.``-`:MMN/`dMMN.` `oMMN:.oMMM+`../:/++:-..yMMd`./ossssmMMNs`  \n", 93);
    sys_print("   ```  `:MMMNNNNNMMMMm+``:mMMNh/-:dMNs`.dMMN.` `oMMN:./MMMd/.-:/:-/hNhmMMm:.:MMMMMMMMNd.`  \n", 93);
    sys_print("   ```  `:NNNNNNNNmho:``  `.+ymNm:::y:```+ooo`` `:ooo-``:o/-:::-.`+mNNNmy+.``yhhhhyyyo/.-`  \n", 93);
    sys_print("    ``  `````````````   ``  ````--.:.`                 `-:::-.`  ````````` `````````````.`  \n", 93);
    sys_print("                            ```  ``--`              `-::::-.`                               \n", 93);
    sys_print("                              `    .::`          `-/+/:-.`                                  \n", 93);
    sys_print("                                   `-/:.      `./ss+:-.`                                    \n", 93);
    sys_print("                                    .:o-`  `.:sdy+:-`                                       \n", 93);
    sys_print("                                    `-ss-.:smNh+:-`                 DoritOS                 \n", 93);
    sys_print("                                     ./mydMNy/:.`                                           \n", 93);
    sys_print("                                     `-yMNy/-.`                                             \n", 93);
    sys_print("                                      .:o-.`                                                \n", 93);
    sys_print("                                       ..`   Carl Friess - Sebastian Winberg - Sven Knobloch\n\n", 94);

}
