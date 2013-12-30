#ifndef AMEGIA_PNP_SERVER_CONTROL_MANAGER_H
#define AMEGIA_PNP_SERVER_CONTROL_MANAGER_H

#include "general_manager.h"

class control_manager: public general_manager {
public:
  static control_manager* get_instance() {
    static control_manager instance;
    return &instance;
  }
};

#endif // AMEGIA_PNP_SERVER_CONTROL_MANAGER_H
