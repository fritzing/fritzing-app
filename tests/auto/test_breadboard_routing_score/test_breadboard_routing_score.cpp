#define BOOST_TEST_MODULE Breadboard routing score tests
#include <boost/test/included/unit_test.hpp>

#include "autoroute/breadboardroutingscore.h"

#include <algorithm>
#include <vector>

BOOST_AUTO_TEST_CASE(completion_is_mandatory)
{
	BreadboardRoutingScore complete;
	complete.jumperCount = 20;
	complete.jumperLength = 2000;

	BreadboardRoutingScore incomplete;
	incomplete.failedNets = 1;
	incomplete.jumperCount = 1;
	incomplete.jumperLength = 10;

	BOOST_CHECK(complete < incomplete);
}

BOOST_AUTO_TEST_CASE(jumper_count_precedes_all_lengths)
{
	BreadboardRoutingScore fewerJumpers;
	fewerJumpers.jumperCount = 2;
	fewerJumpers.jumperLength = 1000;
	fewerJumpers.componentLeadLength = 1000;

	BreadboardRoutingScore moreJumpers;
	moreJumpers.jumperCount = 3;
	moreJumpers.jumperLength = 1;
	moreJumpers.componentLeadLength = 1;

	BOOST_CHECK(fewerJumpers < moreJumpers);
}

BOOST_AUTO_TEST_CASE(jumper_length_precedes_component_lead_length)
{
	BreadboardRoutingScore shorterJumpers;
	shorterJumpers.jumperCount = 4;
	shorterJumpers.jumperLength = 100;
	shorterJumpers.componentLeadLength = 500;

	BreadboardRoutingScore shorterLeads = shorterJumpers;
	shorterLeads.jumperLength = 101;
	shorterLeads.componentLeadLength = 1;

	BOOST_CHECK(shorterJumpers < shorterLeads);
}

BOOST_AUTO_TEST_CASE(component_lead_length_precedes_congestion)
{
	BreadboardRoutingScore shorterLeads;
	shorterLeads.jumperCount = 4;
	shorterLeads.jumperLength = 100;
	shorterLeads.componentLeadLength = 20;
	shorterLeads.congestion = 10000;

	BreadboardRoutingScore cleanerDrawing = shorterLeads;
	cleanerDrawing.componentLeadLength = 21;
	cleanerDrawing.congestion = 0;

	BOOST_CHECK(shorterLeads < cleanerDrawing);
}

BOOST_AUTO_TEST_CASE(parameter_sweep_sorts_by_the_required_objective)
{
	std::vector<BreadboardRoutingScore> results(4);
	results[0] = { 0, 8, 400, 80, 0 };
	results[1] = { 0, 7, 900, 90, 1000 };
	results[2] = { 1, 2, 40, 10, 0 };
	results[3] = { 0, 7, 700, 120, 2000 };

	std::sort(results.begin(), results.end());

	BOOST_CHECK_EQUAL(results[0].jumperCount, 7);
	BOOST_CHECK_EQUAL(results[0].jumperLength, 700);
	BOOST_CHECK_EQUAL(results[1].jumperCount, 7);
	BOOST_CHECK_EQUAL(results[2].jumperCount, 8);
	BOOST_CHECK_EQUAL(results[3].failedNets, 1);
}
