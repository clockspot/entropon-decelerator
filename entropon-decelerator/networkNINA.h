#ifndef NETWORK_H
#define NETWORK_H

void initNetwork();
void cycleNetwork();

void networkStartWiFi();
void networkDisconnectWiFi();
unsigned long ntpSyncAgo();
void cueNTP();
int startNTP(bool synchronous);
bool checkNTP();
void clearNTPSyncLast();

void printCertificate(int secsSpent, int secsSaved);

#endif //NETWORK_H