#include "gloghelper.h"
#include "message_server_controller.h"

int main()
{
  gloghelper::get_instance();
  message_server_controller::get_instance()->run();

  return 0;
}
