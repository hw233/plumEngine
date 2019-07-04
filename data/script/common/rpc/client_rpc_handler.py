# -*- coding: utf-8 -*-

# Author : Yoray
# Created: 2018-10-16 15:36

from __future__ import unicode_literals

import script.object_factory as ModObjFac
import cPickle
import script.common.log as logger
from script.common.rpc.rpcbase import IBaseRpc


class ClientRPCHandler(IBaseRpc):
    def __init__(self, *args):
        IBaseRpc.__init__(self, *args)
        self.client_type = -1

    def set_args(self, client_type, *args):
        self.client_type = client_type
        self.args = args

    def _SendCall(self, msg):
        logger.GetLog().debug('ClientRPCHandler _SendCall : %s' % msg)
        msg_str = cPickle.dumps(msg)
        ModObjFac.CreateApp().send_msg_to_server_node(self.client_type, msg_str, *self.args)