/**
 *******************************************************************************
 * @file    comm_cdc_impl.cpp
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/8/21
 * @version 1.0
 *******************************************************************************
 */




/* ------- define --------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "comm-intf.h"
#include "usbd_cdc_if.h"
#include "../../Services/serv-time.h"


/* ------- class prototypes-----------------------------------------------------------------------------------------*/

class CommCdcImpl : public IComm {
  CommErr transmit(uint8_t*, std::size_t) override;
  CommErr receive(uint8_t*, std::size_t) override{ return CommErr::COMM_SUCCESS;}
  CommErr configure() override{return CommErr::COMM_SUCCESS;}
};

class CommCdcFcty : public ICommFcty {
  IComm* produce() override {
    return new CommCdcImpl();
  }
  IComm* produce(void *handle) override {}
};



/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/

CommCdcFcty cdc_fcty;
ICommFcty* p_cdc_fcty = &cdc_fcty;




/* ------- function implement ----------------------------------------------------------------------------------------*/

CommErr CommCdcImpl::transmit(uint8_t*msg, std::size_t size) {
    auto time = timeServ.getGlobalTimeUs();
 while ( CDC_Transmit_HS(msg, size) != USBD_OK) {

 }
  return CommErr::COMM_SUCCESS;
}