#!/bin/bash
cd nodeManager && gcc Main.c -o "../bin/nodeManager"
cd ../NFT && g++ Main.cpp Client.cpp FileIO.cpp Log.cpp -lpthread -o "../bin/NFT"
echo done
