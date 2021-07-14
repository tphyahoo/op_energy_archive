#!/usr/bin/python

import os, sys
import json, re, random

from urllib2 import build_opener

# Makes a request to a given URL (first arg) and optional params (second arg)
def make_request(*args):
    opener = build_opener()
    opener.addheaders = [('User-agent',
                          'Mozilla/5.0'+str(random.randrange(1000000)))]
    try:
        return opener.open(*args).read().strip()
    except Exception as e:
        try:
            p = e.read().strip()
        except:
            p = e
        raise Exception(p)


def get_block_at_height(height):
    j = json.loads(make_request("https://blockchain.info/block-height/" +
                   str(height)+"?format=json").decode("utf-8"))
    for b in j['blocks']:
        if b['main_chain'] is True:
            return b
    raise Exception("Block at this height not found")


if __name__ == '__main__':

    btc_height_val = 100000     ## default block to retrieve
    if len(sys.argv) > 1:
        btc_height_val = int(sys.argv[1])

    res = get_block_at_height( btc_height_val )
    print( str(res))

