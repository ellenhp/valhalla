#include "gurka.h"
#include <boost/format.hpp>
#include <gtest/gtest.h>

#if !defined(VALHALLA_SOURCE_DIR)
#define VALHALLA_SOURCE_DIR
#endif

using namespace valhalla;
const std::unordered_map<std::string, std::string> build_config{
    {"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/netherlands_admin.sqlite"}}};

class ViaWaypoints : public testing::Test {
protected:
  static gurka::map map;

  static void SetUpTestSuite() {
    constexpr double gridsize_metres = 2;
    const std::string ascii_map = R"(
    A----B----C----D----1E
              |
              |
              F----2G
              |
              |
              J----K---3L
    )";

    const gurka::ways ways = {{"AB", {{"highway", "primary"}}}, {"BC", {{"highway", "primary"}}},
                              {"CD", {{"highway", "primary"}}}, {"DE", {{"highway", "primary"}}},
                              {"CF", {{"highway", "primary"}}}, {"FJ", {{"highway", "primary"}}},
                              {"FG", {{"highway", "primary"}}}, {"JK", {{"highway", "primary"}}},
                              {"KL", {{"highway", "primary"}}}};

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {5.1079374, 52.0887174});
    map = gurka::buildtiles(layout, ways, {}, {}, "test/data/gurka_via_waypoints", build_config);
  }
};
gurka::map ViaWaypoints::map = {};
Api api;
rapidjson::Document d;

// Request:
// /route?json={"locations":[{"lon":5.1079374,"lat":52.0887174,"type":"break"},{"lon":5.109734024089489,"lat":52.087819087955299,"type":"via"},{"lon":5.111530648178978,"lat":52.0887174,"type":"break"}],"format":"osrm","costing":"auto","costing_options":{"auto":{"speed_types":["freeflow","constrained","predicted"]}},"verbose":true,"shape_match":"map_snap"}
/*************************************************************/

 TEST_F(ViaWaypoints, test_via_waypoints_response1) {
  auto result = gurka::do_action(valhalla::Options::route, map, {"A", "1", "L"}, "auto",
                                 {{"/locations/0/type", "break_through"},
                                  {"/locations/1/type", "through"},
                                  {"/locations/2/type", "break_through"}});
  auto d = gurka::convert_to_json(result, valhalla::Options_Format_osrm);

  gurka::assert::raw::expect_path(result,
                                  {"AB", "BC", "CD", "DE", "DE", "CD", "CF", "FJ", "JK",
                                  "KL"});

  ASSERT_EQ(d["routes"].Size(), 1);
  ASSERT_EQ(d["routes"][0]["legs"].Size(), 1);
  ASSERT_EQ(d["routes"][0]["legs"][0]["via_waypoints"].Size(), 1);

  // Expect via waypoint array at leg level
  auto leg = d["routes"][0]["legs"][0].GetObject();
  auto via_waypoint1 = leg["via_waypoints"].GetArray();
  ASSERT_EQ(via_waypoint1.Size(), 1);
  EXPECT_EQ(via_waypoint1[0]["waypoint_index"].GetInt(), 1);
  EXPECT_EQ(via_waypoint1[0]["geometry_index"].GetInt(), 5);
  EXPECT_NEAR(via_waypoint1[0]["distance_from_leg_start"].GetDouble(), 33, 1);
}

TEST_F(ViaWaypoints, test_via_waypoints_response2) {
  auto result = gurka::do_action(valhalla::Options::route, map, {"A", "2", "3", "L"}, "auto",
                                 {{"/locations/0/type", "break_through"},
                                  {"/locations/1/type", "through"},
                                  {"/locations/2/type", "through"},
                                  {"/locations/3/type", "break_through"}});
  auto d = gurka::convert_to_json(result, valhalla::Options_Format_osrm);

  gurka::assert::raw::expect_path(result, {"AB", "BC", "CF", "FG", "FG", "FJ", "JK", "KL"});

  ASSERT_EQ(d["routes"].Size(), 1);
  ASSERT_EQ(d["routes"][0]["legs"].Size(), 1);
  ASSERT_EQ(d["routes"][0]["legs"][0]["via_waypoints"].Size(), 2);

  // Expect via waypoint array at leg level
  auto leg = d["routes"][0]["legs"][0].GetObject();

  auto via_waypoint = leg["via_waypoints"].GetArray();
  ASSERT_EQ(via_waypoint.Size(), 2);
  EXPECT_EQ(via_waypoint[0]["waypoint_index"].GetInt(), 1);
  EXPECT_EQ(via_waypoint[0]["geometry_index"].GetInt(), 5);
  EXPECT_NEAR(via_waypoint[0]["distance_from_leg_start"].GetDouble(), 33, 1.0);
  EXPECT_EQ(via_waypoint[1]["waypoint_index"].GetInt(), 2);
  EXPECT_EQ(via_waypoint[1]["geometry_index"].GetInt(), 8);

  // Compare the last via distance_from_leg_start with overall route-length, they should be similar
  const float route_distance = 50.0;
  ASSERT_NEAR(d["routes"][0]["distance"].GetDouble(), route_distance, 1.0);
  EXPECT_NEAR(via_waypoint[1]["distance_from_leg_start"].GetDouble(), route_distance, 2.0);
}
