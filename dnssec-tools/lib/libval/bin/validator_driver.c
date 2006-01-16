/*
 * Copyright 2005 SPARTA, Inc.  All rights reserved.
 * See the COPYING file distributed with this software for details.
 */ 

/*
 * A command-line validator
 *
 * This program validates the <class, type, domain name> query given
 * on the command line, or runs a set of pre-defined test cases if
 * no command line parameters are given
 *
 * It generates an output suitable for consumption by the
 * drawvalmap.pl script.  This output is written to stderr.
 */

#include <stdio.h>
#include <arpa/nameser.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <resolver.h>
#include <resolv.h>
#include <validator.h>
#include <getopt.h>

#define MAX_RESULTS 10 
#define BUFLEN 16000

int MAX_RESPCOUNT = 10;
int MAX_RESPSIZE = 8192;

// Program options
static struct option prog_options[] = {
		   {"help",  0, 0, 'h'},
                   {"print", 0, 0, 'p'},
                   {"class", 1, 0, 'c'},
                   {"type",  1, 0, 't'},
		   {0, 0, 0, 0}
               };

struct testcase_st {
	const char *desc;
	const char *qn;
	const u_int16_t qc;	
	const u_int16_t qt;	
	const int qr[MAX_RESULTS];
};

// A set of pre-defined test cases
static const struct testcase_st testcases[] = {
#if 1
	{"Test Case 1", "good-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, 0}},
	{"Test Case 2", "badsign-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 3", "nosig-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, 0}},
	{"Test Case 4", "baddata-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 5", "futuredate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 6", "pastdate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 7", "good-cname-to-good-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, VALIDATE_SUCCESS, 0}},
	{"Test Case 8", "good-cname-to-badsign-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, BOGUS_PROVABLE, 0}},
	{"Test Case 9", "good-cname-to-nosig-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, VALIDATION_ERROR, 0}},
	{"Test Case 10", "good-cname-to-baddata-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, BOGUS_PROVABLE, 0}},
	{"Test Case 11", "good-cname-to-futuredate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, BOGUS_PROVABLE, 0}},
	{"Test Case 12", "good-cname-to-pastdate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, BOGUS_PROVABLE, 0}},
	{"Test Case 13", "badsign-cname-to-good-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 14", "badsign-cname-to-badsign-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 15", "badsign-cname-to-nosig-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATION_ERROR, 0}},
	{"Test Case 16", "badsign-cname-to-baddata-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 17", "badsign-cname-to-futuredate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 18", "badsign-cname-to-pastdate-A.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 19", "nosig-cname-to-good-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, VALIDATE_SUCCESS, 0}},
	{"Test Case 20", "nosig-cname-to-badsign-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, BOGUS_PROVABLE, 0}},
	{"Test Case 21", "nosig-cname-to-nosig-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, VALIDATION_ERROR, 0}},
	{"Test Case 22", "nosig-cname-to-baddata-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, BOGUS_PROVABLE, 0}},
	{"Test Case 23", "nosig-cname-to-futuredate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, BOGUS_PROVABLE, 0}},
	{"Test Case 24", "nosig-cname-to-pastdate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, BOGUS_PROVABLE, 0}},
	{"Test Case 25", "baddata-cname-to-good-A.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 26", "baddata-cname-to-badsign-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 27", "baddata-cname-to-nosig-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 28", "baddata-cname-to-baddata-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 29", "baddata-cname-to-futuredate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 30", "baddata-cname-to-pastdate-A.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 31", "futuredate-cname-to-good-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 32", "futuredate-cname-to-badsign-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 33", "futuredate-cname-to-nosig-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATION_ERROR, 0}},
	{"Test Case 34", "futuredate-cname-to-baddata-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 35", "futuredate-cname-to-futuredate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 36", "futuredate-cname-to-pastdate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 37", "pastdate-cname-to-good-A.test.dnssec-tools.org", ns_c_in,	ns_t_a, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 38", "pastdate-cname-to-badsign-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 39", "pastdate-cname-to-nosig-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, VALIDATION_ERROR, 0}},
	{"Test Case 40", "pastdate-cname-to-baddata-A.test.dnssec-tools.org", ns_c_in, 	ns_t_a,	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 41", "pastdate-cname-to-futuredate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 42", "pastdate-cname-to-pastdate-A.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 43", "good-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATE_SUCCESS, 0}},
	{"Test Case 44", "badsign-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {BOGUS_PROVABLE, 0}},
	{"Test Case 45", "nosig-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATION_ERROR, 0}},
	{"Test Case 46", "baddata-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {BOGUS_PROVABLE, 0}},
	{"Test Case 47", "futuredate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},
	{"Test Case 48", "pastdate-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {BOGUS_PROVABLE, 0}},
	{"Test Case 49", "good-cname-to-good-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATE_SUCCESS, VALIDATE_SUCCESS, 0}},
	{"Test Case 50", "good-cname-to-badsign-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATE_SUCCESS, BOGUS_PROVABLE, 0}},
	{"Test Case 51", "good-cname-to-nosig-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATE_SUCCESS, VALIDATION_ERROR, 0}},
	{"Test Case 52", "good-cname-to-baddata-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATE_SUCCESS, BOGUS_PROVABLE, 0}},
	{"Test Case 53", "good-cname-to-futuredate-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATE_SUCCESS, BOGUS_PROVABLE, 0}},
	{"Test Case 54", "good-cname-to-pastdate-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATE_SUCCESS, BOGUS_PROVABLE, 0}},
	{"Test Case 55", "badsign-cname-to-good-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 56", "badsign-cname-to-badsign-AAAA.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 57", "badsign-cname-to-nosig-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, VALIDATION_ERROR, 0}},
	{"Test Case 58", "badsign-cname-to-baddata-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 59", "badsign-cname-to-futuredate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 60", "badsign-cname-to-pastdate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 61", "nosig-cname-to-good-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{VALIDATION_ERROR, VALIDATE_SUCCESS, 0}},
	{"Test Case 62", "nosig-cname-to-badsign-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{VALIDATION_ERROR, BOGUS_PROVABLE, 0}},
	{"Test Case 63", "nosig-cname-to-nosig-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{VALIDATION_ERROR, VALIDATION_ERROR, 0}},
	{"Test Case 64", "nosig-cname-to-baddata-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{VALIDATION_ERROR, BOGUS_PROVABLE, 0}},
	{"Test Case 65", "nosig-cname-to-futuredate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{VALIDATION_ERROR, BOGUS_PROVABLE, 0}},
	{"Test Case 66", "nosig-cname-to-pastdate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{VALIDATION_ERROR, BOGUS_PROVABLE, 0}},
	{"Test Case 67", "baddata-cname-to-good-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 68", "baddata-cname-to-badsign-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 69", "baddata-cname-to-nosig-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 70", "baddata-cname-to-baddata-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 71", "baddata-cname-to-futuredate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 72", "baddata-cname-to-pastdate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 73", "futuredate-cname-to-good-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 74", "futuredate-cname-to-badsign-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 75", "futuredate-cname-to-nosig-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, VALIDATION_ERROR, 0}},
	{"Test Case 76", "futuredate-cname-to-baddata-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 77", "futuredate-cname-to-futuredate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 78", "futuredate-cname-to-pastdate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 79", "pastdate-cname-to-good-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, VALIDATE_SUCCESS, 0}},
	{"Test Case 80", "pastdate-cname-to-badsign-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 81", "pastdate-cname-to-nosig-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, VALIDATION_ERROR, 0}},
	{"Test Case 82", "pastdate-cname-to-baddata-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 83", "pastdate-cname-to-futuredate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 84", "pastdate-cname-to-pastdate-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, BOGUS_PROVABLE, 0}},
	{"Test Case 85", "good-A.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, 0}},
	{"Test Case 86", "badsign-A.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 87", "nosig-A.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, 0}},
	{"Test Case 88", "baddata-A.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 89", "futuredate-A.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 90", "pastdate-A.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 91", "good-AAAA.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATE_SUCCESS, 0}},
	{"Test Case 92", "badsign-AAAA.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {BOGUS_PROVABLE, 0}},
	{"Test Case 93", "nosig-AAAA.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {VALIDATION_ERROR, 0}},
	{"Test Case 94", "baddata-AAAA.good-ns.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {BOGUS_PROVABLE, 0}},
	{"Test Case 95", "futuredate-AAAA.good-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},
	{"Test Case 96", "pastdate-AAAA.good-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, 0}},
	{"Test Case 97", "addedlater-A.good-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {NONEXISTENT_NAME, NONEXISTENT_NAME, 0}},
	{"Test Case 98", "addedlater-AAAA.good-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {NONEXISTENT_NAME, NONEXISTENT_NAME, 0}},
	{"Test Case 99", "good-A.badsign-ns.test.dnssec-tools.org", ns_c_in, ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 100", "badsign-A.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 101", "nosig-A.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}},
	{"Test Case 102", "baddata-A.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 103", "futuredate-A.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 104", "pastdate-A.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 105", "good-AAAA.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, 0}},
	{"Test Case 106", "badsign-AAAA.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, 0}},
	{"Test Case 107", "nosig-AAAA.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{VALIDATION_ERROR, 0}},
	{"Test Case 108", "baddata-AAAA.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, 0}},
	{"Test Case 109", "futuredate-AAAA.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, 0}},
	{"Test Case 110", "pastdate-AAAA.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROVABLE, 0}},
	{"Test Case 111", "addedlater-A.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 112", "addedlater-AAAA.badsign-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 113", "good-A.nosig-ns.test.dnssec-tools.org", ns_c_in, ns_t_a, {VALIDATION_ERROR, 0}},
	{"Test Case 114", "badsign-A.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, 	{VALIDATION_ERROR, 0}},
	{"Test Case 115", "nosig-A.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}},
	{"Test Case 116", "baddata-A.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}},	
	{"Test Case 117", "futuredate-A.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}},
	{"Test Case 118", "pastdate-A.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}},	
	{"Test Case 119", "good-AAAA.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},
	{"Test Case 120", "badsign-AAAA.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 121", "nosig-AAAA.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 122", "baddata-AAAA.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 123", "futuredate-AAAA.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 124", "pastdate-AAAA.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 125", "addedlater-A.nosig-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 126", "addedlater-AAAA.nosig-ns.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, {BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 127", "good-A.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}}, 
	{"Test Case 128", "badsign-A.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}}, 
	{"Test Case 129", "nosig-A.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}}, 
	{"Test Case 130", "baddata-A.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}}, 
	{"Test Case 131", "futuredate-A.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}},
	{"Test Case 132", "pastdate-A.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}}, 
	{"Test Case 133", "good-AAAA.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 134", "badsign-AAAA.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}}, 
	{"Test Case 135", "nosig-AAAA.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}}, 
	{"Test Case 136", "baddata-AAAA.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 137", "futuredate-AAAA.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}}, 
	{"Test Case 138", "pastdate-AAAA.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 139", "addedlater-A.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 140", "addedlater-AAAA.nods-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 141", "good-A.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},	
	{"Test Case 142", "badsign-A.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}}, 
	{"Test Case 143", "nosig-A.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}},
	{"Test Case 144", "baddata-A.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 145", "futuredate-A.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}}, 
	{"Test Case 146", "pastdate-A.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},
	{"Test Case 147", "good-AAAA.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},	
	{"Test Case 148", "badsign-AAAA.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}}, 
	{"Test Case 149", "nosig-AAAA.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}},	
	{"Test Case 150", "baddata-AAAA.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},	
	{"Test Case 151", "futuredate-AAAA.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},	
	{"Test Case 152", "pastdate-AAAA.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}}, 
	{"Test Case 153", "addedlater-A.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a,	{BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 154", "addedlater-AAAA.futuredate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 155", "good-A.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}}, 	
	{"Test Case 156", "badsign-A.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},	
	{"Test Case 157", "nosig-A.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {VALIDATION_ERROR, 0}}, 
	{"Test Case 158", "baddata-A.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}},	
	{"Test Case 159", "futuredate-A.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}}, 
	{"Test Case 160", "pastdate-A.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a, {BOGUS_PROVABLE, 0}}, 
	{"Test Case 161", "good-AAAA.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},	
	{"Test Case 162", "badsign-AAAA.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},	
	{"Test Case 163", "nosig-AAAA.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {VALIDATION_ERROR, 0}}, 
	{"Test Case 164", "baddata-AAAA.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},	
	{"Test Case 165", "futuredate-AAAA.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}}, 
	{"Test Case 166", "pastdate-AAAA.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, {BOGUS_PROVABLE, 0}},	
	{"Test Case 167", "addedlater-A.pastdate-ns.test.dnssec-tools.org", ns_c_in, 	ns_t_a,	{BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 168", "addedlater-AAAA.pastdate-ns.test.dnssec-tools.org", ns_c_in, ns_t_aaaa, 	{BOGUS_PROOF, BOGUS_PROOF, 0}},
	{"Test Case 169", "addedlater-A.test.dnssec-tools.org", ns_c_in, 	ns_t_a, 	{NONEXISTENT_NAME, NONEXISTENT_NAME, 0}},
	{"Test Case 170", "addedlater-AAAA.test.dnssec-tools.org", ns_c_in, 	ns_t_aaaa, 	{NONEXISTENT_NAME, NONEXISTENT_NAME, 0}},
#endif

#if 0
	#if 1
	/* Test for resolution error (ensure no "search" in resolv.conf) */
	{"Checking name failure", "dns", ns_c_in, ns_t_a, {DNS_ERROR_BASE+SR_SERVFAIL, 0}},
	#endif

	#if 1
	/* Test for non-existence */
	{"Checking non-existence proofs", "dns1.wesh.fruits.netsec.tislabs.com.", ns_c_in, ns_t_a, {NONEXISTENT_NAME, NONEXISTENT_NAME, NONEXISTENT_NAME, 0}}, 
	#endif

	#if 1
	/* Test for validation without recursion + CNAME */
	{"Testing CNAME and same-level validation", "apple.fruits.netsec.tislabs.com.", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, VALIDATE_SUCCESS, 0}},
	#endif

	#if 1
	/* Test for validation with recursion */
	{"Testing validation up the chain", "dns.wesh.fruits.netsec.tislabs.com.", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, 0}},
	#endif

	#if 1
	/* Test for multiple answers */
	{"Checking validation of multiple answers returned with ANY", "fruits.netsec.tislabs.com.", ns_c_in, ns_t_any, {VALIDATE_SUCCESS, VALIDATE_SUCCESS, VALIDATE_SUCCESS, VALIDATE_SUCCESS, VALIDATE_SUCCESS, VALIDATE_SUCCESS, 0}},
	#endif

	#if 1
	/* Wild-card test */
	{"Checking validation with a wildcard match", "jackfruit.fruits.netsec.tislabs.com.", ns_c_in, ns_t_a, {VALIDATE_SUCCESS, 0}},
	#endif

	#if 1
	/* Wild-card, non-existent type */
	{"Checking if wildcard with a different type matches", "jackfruit.fruits.netsec.tislabs.com.", ns_c_in, ns_t_cname, {DNS_ERROR_BASE+SR_NO_ANSWER, 0}}, 
	#endif

	#if 0
	/* Test for bad class */
	{"Testing bad class", "dns.wesh.fruits.netsec.tislabs.com.", 15, ns_t_a, {DNS_ERROR_BASE+SR_NO_ANSWER, 0}},
	#endif
#endif

	{NULL, NULL, 0, 0, {0}},
};


// A wrapper function to send a query and print the output onto stderr
//
void sendquery(const char *desc, const char *name, const u_int16_t class, const u_int16_t type, const int result_ar[])
{
	int ret_val;
    struct query_chain *queries = NULL;
    struct assertion_chain *assertions = NULL;
    struct val_result *results = NULL;
	struct val_result *res;
    val_context_t *context;
    u_char name_n[MAXCDNAME];
	int err = 0;
	int result_array[MAX_RESULTS];
	int i;

	fprintf(stderr, "%s:\t", desc);

    if(NO_ERROR !=(ret_val = get_context(NULL, &context))) {
		fprintf(stderr, "Error: %d\n", ret_val);		
        return;
	}

    if (ns_name_pton(name, name_n, MAXCDNAME-1) == -1) {
		fprintf(stderr, "Error: %d\n", BAD_ARGUMENT);		
        return;
	}                                                                                                                 

	ret_val = resolve_n_check(context, name_n, type, class, 0,  
                                   &queries, &assertions, &results);


	/* make a local copy of result array */
	i = 0;
	while(result_ar[i] != 0) {
		result_array[i] = result_ar[i];
		i++;
	}
	result_array[i] = 0;

	if (ret_val == NO_ERROR) {
		for (res=results; res; res=res->next) {
			for(i=0; result_array[i]!=0; i++) {
				if(res->status == result_array[i]) {
					/* Mark this as done */
					result_array[i] = -1;
					break;
				}
			}
			if (result_array[i]==0) {
				fprintf(stderr, "FAILED: Remaining error values expected\n"); 
				for(i=0; result_array[i]!=0; i++) {
					if(result_array[i]!=-1)
						fprintf(stderr, "     %s(%d)\n", p_val_error(result_array[i]), result_array[i]);	
				}
				fprintf(stderr, "\n");
				val_log_assertion_chain(context, LOG_INFO, name_n, class, type, queries, results);
				err = 1;
			}
		}

		/* All results were in the result array */
		if(!err) { 
			/* Check if all error values were marked */
			for(i=0; result_array[i]!=0; i++) {
				if(result_array[i]!= -1) {
					fprintf(stderr, "FAILED: Some results were not received \n");
					val_log_assertion_chain(context, LOG_INFO, name_n, class, type, queries, results);
					err = 1;
					break;
				}
			}

			if (!err) {
				fprintf(stderr, "OK\n");
				val_log_assertion_chain(context, LOG_INFO, name_n, class, type, queries, results);
			}
		}
	}
	else {
		printf ("FAILED: Error in resolve_n_check(): %d\n", ret_val);
	}

    /* XXX De-register pending queries */
    free_query_chain(&queries);
    free_assertion_chain(&assertions);
    free_result_chain(&results);

    destroy_context(context);
}

// Usage
void usage(char *progname)
{
	printf("Usage: validate [options] [DOMAIN_NAME]\n");
	printf("Resolve and validate a DNS query.\n");
	printf("Options:\n");
	printf("        -h, --help             Display this help and exit\n");
	printf("        -p, --print            Print the answer and validation result\n");
	printf("        -c, --class=<CLASS>    Specifies the class (default IN)\n");
	printf("        -t, --type=<TYPE>      Specifies the type (default A)\n");
	printf("\nThe DOMAIN_NAME parameter is not required for the -h option.\n");
	printf("The DOMAIN_NAME parameter is required if one of -p, -c or -t options is given.\n");
	printf("If no arguments are given, this program runs a set of predefined test queries.\n");
}

// Main
int main(int argc, char *argv[])
{
	if (argc == 1) {
		// Run the set of pre-defined test cases
		int i;
		for (i= 0 ; testcases[i].desc != NULL; i++) {
			sendquery(testcases[i].desc, testcases[i].qn, testcases[i].qc, testcases[i].qt, testcases[i].qr);
			fprintf(stderr, "\n");
		}
	}
	else {
		// Parse the command line for a query and resolve+validate it
		int c;
		char *classstr    = NULL;
		char *typestr     = NULL;
		char *domain_name = NULL;
		u_int16_t class_h = ns_c_in;
		u_int16_t type_h  = ns_t_a;
		int success       = 0;
		int doprint       = 0;
		int retvals[]     = {VALIDATE_SUCCESS, 0};

		while (1) {
			int opt_index     = 0;
			c = getopt_long_only (argc, argv, "pc:t:",
					      prog_options, &opt_index);

			if (c == -1) {
				break;
			}

			switch(c) {
			case 'h':
			        usage(argv[0]);
			        return(0);

			case 'p':
				doprint = 1;
				break;

			case 'c':
				// optarg is a global variable.  See man page for getopt_long(3).
				class_h = res_nametoclass(optarg, &success);
				if (!success) {
					fprintf(stderr, "Cannot parse class %s\n", optarg);
					usage(argv[0]);
					return 1;
				}
				break;
			case 't':
				type_h = res_nametotype(optarg, &success);
				if (!success) {
					fprintf(stderr, "Cannot parse type %s\n", optarg);
					usage(argv[0]);
					return 1;
				}
				break;
			default:
				fprintf(stderr, "Unknown option %s (c = %d [%c])\n", argv[optind - 1], c, (char) c);
				usage(argv[0]);
				return 1;

			} // end switch
		}

		// optind is a global variable.  See man page for getopt_long(3)
		if (optind < argc) {
			domain_name = argv[optind++];
			sendquery("Result", domain_name, class_h, type_h,
				  retvals);
			fprintf(stderr, "\n");

			// If the print option is present, perform query and validation again for printing the result
			if (doprint) {
				int retval = 0;
				int dnssec_status = -1;
				struct val_response resp[MAX_RESPCOUNT];
				int resp_count = MAX_RESPCOUNT;
				int i;
				for (i=0; i<resp_count; i++) {
				    bzero(&resp[i], sizeof (struct val_response));
				    resp[i].response = (u_int8_t *) malloc (MAX_RESPSIZE * sizeof(u_int8_t));
				    if (resp[i].response == NULL) {
					    printf("Memory allocation error. Please try again later.\n");
					    exit(1);
				    }
				    bzero(resp[i].response, MAX_RESPSIZE * sizeof(u_int8_t));
				    resp[i].response_length = MAX_RESPSIZE;
				}

				retval = val_query(NULL, domain_name, class_h, type_h, 0, resp, &resp_count);
				
				if (resp_count > 0) {
					for (i=0; i<resp_count; i++) {
						
						printf("DNSSEC status: %s [%d]\n", p_val_error(resp[i].val_status),
						       resp[i].val_status);
						if (resp[i].val_status == VALIDATE_SUCCESS) {
							printf("Validated response:\n");
						}
						else {
							printf("Non-validated response:\n");
						}
						print_response(resp[i].response, resp[i].response_length);
						printf("\n");
					}
				}
				else {
					printf("No answers returned. [resp_count = %d, retval = %d]\n", resp_count, retval);
				}
			}
		}
		else {
			fprintf(stderr, "Please specify domain name\n");
			usage(argv[0]);
			return 1;
		}
	}

	return 0;
}
