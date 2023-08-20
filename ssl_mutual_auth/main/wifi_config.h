#ifndef  __APP_CONFIG_H
#define __APP_CONFIG_H

typedef enum {
    PROVISION_ACCESSPOINT = 0,
    PROVISION_SMARTCONFIG = 1
}   provision_type_t;

void wifi_config(void);

#endif