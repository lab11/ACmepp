#!/usr/bin/env python3

import asyncio
import aiocoap as coap
import json
import sys


def main ():

	protocol = yield from coap.Context.create_client_context()

	if len(sys.argv) == 1:

		request = coap.Message(code=coap.GET)
		request.set_request_uri('coap://[2607:f018:800:111:c298:e541:4310:d]/onoffdevice')

		response = yield from protocol.request(request).response

		power = json.loads(response.payload.decode('utf-8'))
		print('The device is {}'.format(('off', 'on')[power['Power']]))

	else:
		if sys.argv[1] == 'on':
			request = coap.Message(code=coap.POST, payload=b'Power=true')
			request.set_request_uri('coap://[2607:f018:800:111:c298:e541:4310:d]/onoffdevice')
			response = yield from protocol.request(request).response
			print('Device turned on')

		elif sys.argv[1] == 'off':
			request = coap.Message(code=coap.POST, payload=b'Power=false')
			request.set_request_uri('coap://[2607:f018:800:111:c298:e541:4310:d]/onoffdevice')
			response = yield from protocol.request(request).response
			print('Device turned off')

		else:
			print('control.py [on|off]')



asyncio.get_event_loop().run_until_complete(main())

