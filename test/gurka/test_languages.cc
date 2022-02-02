#include "gurka.h"
#include "test/test.h"
#include <gtest/gtest.h>

#if !defined(VALHALLA_SOURCE_DIR)
#define VALHALLA_SOURCE_DIR
#endif

using namespace valhalla;
using namespace valhalla::baldr;
using namespace valhalla::gurka;

valhalla::gurka::map the_map = {};

class RouteWithStreetnameAndSign_en_UnitedStates : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       J
                       |
                       |
                       |
                       I
                      /|\
                    /  |  \
                  /    |    \
           L----K-------------H----G
           A----B-------------E----F
                  \    |    /
                    \  |  /
                      \|/
                       C
                       |
                       |
                       |
                       D
               O------PM------Q
                       |
                       |
                       |
                       N

    )";

    const gurka::ways ways = {
        {"ABEF", {{"highway", "motorway"}, {"name", ""}, {"ref", "I 70"}, {"oneway", "yes"}}},
        {"GHKL", {{"highway", "motorway"}, {"name", ""}, {"ref", "I 70"}, {"oneway", "yes"}}},
        {"JICDMN",
         {{"highway", "primary"},
          {"osm_id", "100"},
          {"name", "6th Avenue"},
          {"name:ru", "6-я авеню"},
          {"ref", "SR 37"}}},
        {"BC",
         {{"highway", "motorway_link"},
          {"osm_id", "101"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "126B"},
          {"destination", "York;Lancaster"},
          {"destination:lang:ru", "Йорк;Ланкастер"},
          {"destination:street", "6th Avenue"},
          {"destination:street:lang:ru", "6-я авеню"},
          {"destination:ref", "SR 37"}}},
        {"CE",
         {{"highway", "motorway_link"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination:ref", "I 70 East"},
          {"destination:ref:lang:ru", "Я 70 Восток"}}},
        {"HI",
         {{"highway", "motorway_link"},
          {"osm_id", "102"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "126B"},
          {"destination:street:to", "Main Street"},
          {"destination:street:to:lang:ru", "Главная улица"},
          {"destination:ref:to", "I 80"},
          {"destination:ref:to:lang:ru", "Я 80"}}},
        {"IK",
         {{"highway", "motorway_link"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination:ref", "I 70 West"},
          {"destination:ref:lang:ru", "Я 70 Запад"}}},
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", ""},
          {"name:en", "West 8th Street"},
          {"name:ru", "Западная 8-я стрит"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "York"},
          {"destination:street", "West 8th Street"},
          {"destination:street:lang:ru", "Западная 8-я стрит"}}},
    };

    const gurka::nodes nodes = {{"M", {{"highway", "traffic_signals"}, {"name", "M Junction"}}}};

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {-82.68811, 40.22535});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, nodes, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_UnitedStates, CheckStreetNamesAndSigns1) {

  const std::string workdir = "test/data/gurka_language_with_streetname_and_sign_en_UnitedStates";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"A", "D"}, "auto");
  gurka::assert::raw::expect_path(result, {"I 70", "", "6th Avenue/SR 37"});

  // Verify starting on I 70
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "I 70");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets_size(),
            2);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(0)
                .text(),
            "SR 37");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(1)
                .text(),
            "6th Avenue");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            2);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "York");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Lancaster");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId BC_edge_id;
  const DirectedEdge* BC_edge = nullptr;
  GraphId CB_edge_id;
  const DirectedEdge* CB_edge = nullptr;
  std::tie(BC_edge_id, BC_edge, CB_edge_id, CB_edge) =
      findEdge(graph_reader, the_map.nodes, "", "C", baldr::GraphId{}, 101);
  EXPECT_NE(BC_edge, nullptr);
  EXPECT_NE(CB_edge, nullptr);

  GraphId JICDMN_edge_id;
  const DirectedEdge* JICDMN_edge = nullptr;
  GraphId NMDCIJ_edge_id;
  const DirectedEdge* NMDCIJ_edge = nullptr;
  std::tie(JICDMN_edge_id, JICDMN_edge, NMDCIJ_edge_id, NMDCIJ_edge) =
      findEdge(graph_reader, the_map.nodes, "", "N", baldr::GraphId{}, 100);
  EXPECT_NE(JICDMN_edge, nullptr);
  EXPECT_NE(NMDCIJ_edge, nullptr);

  GraphId node_id = BC_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(BC_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(BC_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 5);
  ASSERT_EQ(linguistics.size(), 5);

  ASSERT_EQ(edge_signs.at(0).text(), "126B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(1).text(), "SR 37");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(2).text(), "6th Avenue");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(3).text(), "York");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(4).text(), "Lancaster");
  iter = linguistics.find(4);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "6th Avenue");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "SR 37");

  node_id = JICDMN_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(JICDMN_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();

  ASSERT_EQ(names_and_types.size(), 2);

  ASSERT_EQ(names_and_types.at(0).first, "6th Avenue");
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  ASSERT_EQ(names_and_types.at(1).first, "SR 37");
  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_UnitedStates, CheckStreetNamesAndSigns2) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"G", "J"}, "auto");
  gurka::assert::raw::expect_path(result, {"I 70", "", "6th Avenue/SR 37"});

  // Verify starting on I 70
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "I 70");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            2);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "I 80");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Main Street");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId HI_edge_id;
  const DirectedEdge* HI_edge = nullptr;
  GraphId IH_edge_id;
  const DirectedEdge* IH_edge = nullptr;
  std::tie(HI_edge_id, HI_edge, IH_edge_id, IH_edge) =
      findEdge(graph_reader, the_map.nodes, "", "I", baldr::GraphId{}, 102);
  EXPECT_NE(HI_edge, nullptr);
  EXPECT_NE(IH_edge, nullptr);

  GraphId node_id = HI_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(HI_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(HI_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 3);

  ASSERT_EQ(edge_signs.at(0).text(), "126B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(1).text(), "I 80");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(2).text(), "Main Street");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_UnitedStates, CheckGuideSigns) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"6th Avenue/SR 37", "6th Avenue/SR 37", "6th Avenue/SR 37",
                                           "", "West 8th Street"});

  // Verify starting on 6th Avenue/SR 37
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "6th Avenue");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "SR 37");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets_size(),
            1);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(0)
                .text(),
            "West 8th Street");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations_size(),
            1);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations(0)
                .text(),
            "York");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  GraphId node_id = PD_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(PD_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "West 8th Street");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());

  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(1).text(), "York");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());

  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_UnitedStates, CheckNonJunctionName) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"6th Avenue/SR 37", "6th Avenue/SR 37", "6th Avenue/SR 37",
                                           "6th Avenue/SR 37", "West 8th Street"});

  // Verify starting on 6th Avenue/SR 37
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "6th Avenue");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "SR 37");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "West 8th Street");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "West 8th Street");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  // No junction should exist here.  Named junctions are not allowed in US
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names_size(),
            0);
}

class RouteWithStreetnameAndSign_fr_nl_BrusselsBelgium : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       J
                       |
                       |
                       |
                       I
                      /|\
                    /  |  \
                  /    |    \
           L----K-------------H----G
           A----B-------------E----F
                  \    |    /
                    \  |  /
                      \|/
                       C
                       |
                       |
                       |
                       D
               O------PM------Q
                       |
                       |
                       |
                       N

    )";

    const gurka::ways ways = {
        {"ABEF", {{"highway", "motorway"}, {"name", ""}, {"ref", "E40"}, {"oneway", "yes"}}},
        {"GHKL", {{"highway", "motorway"}, {"name", ""}, {"ref", "E40"}, {"oneway", "yes"}}},
        {"JICDMN",
         {{"highway", "primary"},
          {"osm_id", "100"},
          {"name", "Rue Bodenbroek - Bodenbroekstraat"},
          {"name:fr", "Rue Bodenbroek"},
          {"name:nl", "Bodenbroekstraat"},
          {"ref", "N6"}}},
        {"BC",
         {{"highway", "motorway_link"},
          {"osm_id", "101"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "12"},
          {"destination", "Brussel;Namen"},
          {"destination:street", "Bodenbroekstraat"},
          {"destination:street:lang:fr", "Rue Bodenbroek"},
          {"destination:street:lang:nl", "Bodenbroekstraat"},
          {"destination:ref", "N6"}}},
        {"CE",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "E40"}}},
        {"HI",
         {{"highway", "motorway_link"},
          {"osm_id", "102"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "12"},
          {"destination:street:to", "Koningsstraat"},
          {"destination:street:to:lang:fr", "Rue Royale"},
          {"destination:street:to:lang:nl", "Koningsstraat"},
          {"destination:ref:to", "E19"}}},
        {"IK",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "E40"}}},
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Rue Lebeau - Lebeaustraat"},
          {"name:fr", "Rue Lebeau"},
          {"name:nl", "Lebeaustraat"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "Brussel"},
          {"destination:street", "Lebeaustraat"},
          {"destination:street:lang:fr", "Rue Lebeau"},
          {"destination:street:lang:nl", "Lebeaustraat"}}},
    };

    const gurka::nodes nodes = {{"M", {{"highway", "traffic_signals"}, {"name", "Zaventem"}}}};

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {4.3516970, 50.8465573});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, nodes, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_BrusselsBelgium, CheckStreetNamesAndSigns1) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_fr_nl_BrusselsBelgium";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"A", "D"}, "auto");
  gurka::assert::raw::expect_path(result, {"E40", "", "Rue Bodenbroek/Bodenbroekstraat/N6"});

  // Verify starting on E40
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "E40");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets_size(),
            3);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(0)
                .text(),
            "N6");
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(1)
                .text(),
            "Bodenbroekstraat");
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(2)
                .text(),
            "Rue Bodenbroek");
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            2);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "Brussel");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Namen");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Rue Bodenbroek");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Bodenbroekstraat");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "N6");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId BC_edge_id;
  const DirectedEdge* BC_edge = nullptr;
  GraphId CB_edge_id;
  const DirectedEdge* CB_edge = nullptr;
  std::tie(BC_edge_id, BC_edge, CB_edge_id, CB_edge) =
      findEdge(graph_reader, the_map.nodes, "", "C", baldr::GraphId{}, 101);
  EXPECT_NE(BC_edge, nullptr);
  EXPECT_NE(CB_edge, nullptr);

  GraphId JICDMN_edge_id;
  const DirectedEdge* JICDMN_edge = nullptr;
  GraphId NMDCIJ_edge_id;
  const DirectedEdge* NMDCIJ_edge = nullptr;
  std::tie(JICDMN_edge_id, JICDMN_edge, NMDCIJ_edge_id, NMDCIJ_edge) =
      findEdge(graph_reader, the_map.nodes, "", "N", baldr::GraphId{}, 100);
  EXPECT_NE(JICDMN_edge, nullptr);
  EXPECT_NE(NMDCIJ_edge, nullptr);

  GraphId node_id = BC_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(BC_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(BC_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 6);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "12");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "N6");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Bodenbroekstraat");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "nl");

  ASSERT_EQ(edge_signs.at(3).text(), "Rue Bodenbroek");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(4).text(), "Brussel");
  iter = linguistics.find(4);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(5).text(), "Namen");
  iter = linguistics.find(5);
  ASSERT_EQ(iter, linguistics.end());

  node_id = JICDMN_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(JICDMN_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();

  ASSERT_EQ(names_and_types.size(), 3);

  ASSERT_EQ(names_and_types.at(0).first, "Rue Bodenbroek");
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  ASSERT_EQ(names_and_types.at(1).first, "Bodenbroekstraat");
  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "nl");

  ASSERT_EQ(names_and_types.at(2).first, "N6");
  lang_iter = languages.find(2);
  ASSERT_EQ(lang_iter, languages.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_BrusselsBelgium, CheckStreetNamesAndSigns2) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"G", "J"}, "auto");
  gurka::assert::raw::expect_path(result, {"E40", "", "Rue Bodenbroek/Bodenbroekstraat/N6"});

  // Verify starting on E40
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "E40");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            3);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "E19");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Koningsstraat");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(2)
                .text(),
            "Rue Royale");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId HI_edge_id;
  const DirectedEdge* HI_edge = nullptr;
  GraphId IH_edge_id;
  const DirectedEdge* IH_edge = nullptr;
  std::tie(HI_edge_id, HI_edge, IH_edge_id, IH_edge) =
      findEdge(graph_reader, the_map.nodes, "", "I", baldr::GraphId{}, 102);
  EXPECT_NE(HI_edge, nullptr);
  EXPECT_NE(IH_edge, nullptr);

  GraphId node_id = HI_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(HI_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(HI_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 4);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "12");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "E19");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Koningsstraat");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "nl");

  ASSERT_EQ(edge_signs.at(3).text(), "Rue Royale");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_BrusselsBelgium, CheckGuideSigns) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Rue Bodenbroek/Bodenbroekstraat/N6",
                                           "Rue Bodenbroek/Bodenbroekstraat/N6",
                                           "Rue Bodenbroek/Bodenbroekstraat/N6", "",
                                           "Rue Lebeau/Lebeaustraat"});

  // Verify starting on Rue Bodenbroek/Bodenbroekstraat/N6
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Rue Bodenbroek");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Bodenbroekstraat");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "N6");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets_size(),
            2);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(0)
                .text(),
            "Lebeaustraat");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(1)
                .text(),
            "Rue Lebeau");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations_size(),
            1);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations(0)
                .text(),
            "Brussel");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  GraphId node_id = PD_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(PD_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "Lebeaustraat");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "nl");

  ASSERT_EQ(edge_signs.at(1).text(), "Rue Lebeau");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(2).text(), "Brussel");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_BrusselsBelgium, CheckNonJunctionName) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "Q"}, "auto");
  gurka::assert::raw::expect_path(result,
                                  {"Rue Bodenbroek/Bodenbroekstraat/N6",
                                   "Rue Bodenbroek/Bodenbroekstraat/N6",
                                   "Rue Bodenbroek/Bodenbroekstraat/N6",
                                   "Rue Bodenbroek/Bodenbroekstraat/N6", "Rue Lebeau/Lebeaustraat"});

  // Verify starting on Rue Bodenbroek/Bodenbroekstraat/N6
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Rue Bodenbroek");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Bodenbroekstraat");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "N6");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Rue Lebeau");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Lebeaustraat");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Rue Lebeau");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Lebeaustraat");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "nl");

  // No junction should exist here.  Named junctions are not allowed in Belgium
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names_size(),
            0);
}

class RouteWithStreetnameAndSign_ru_be_MinskBelarus : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       J
                       |
                       |
                       |
                       I
                      /|\
                    /  |  \
                  /    |    \
           L----K-------------H----G
           A----B-------------E----F
                  \    |    /
                    \  |  /
                      \|/
                       C
                       |
                       |
                       |
                       D
               O------PM------Q
                       |
                       |
                       |
                       N

    )";

    const gurka::ways ways = {
        {"ABEF", {{"highway", "motorway"}, {"name", ""}, {"ref", "М2"}, {"oneway", "yes"}}},
        {"GHKL", {{"highway", "motorway"}, {"name", ""}, {"ref", "М2"}, {"oneway", "yes"}}},
        {"JICDMN",
         {{"highway", "primary"},
          {"osm_id", "100"},
          {"name", "МКАД, 1-й километр"},
          {"name:ru", "МКАД, 1-й километр"},
          {"name:be", "1-ы кіламетр МКАД"},
          {"ref", "M9"}}},
        {"BC",
         {{"highway", "motorway_link"},
          {"osm_id", "101"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "12"},
          {"destination", "Гомель;Слуцк"},
          {"destination:street", "1-ы кіламетр МКАД"},
          {"destination:street:lang:ru", "МКАД, 1-й километр"},
          {"destination:street:lang:be", "1-ы кіламетр МКАД"},
          {"destination:ref", "M9"}}},
        {"CE",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "М2"}}},
        {"HI",
         {{"highway", "motorway_link"},
          {"osm_id", "102"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "12"},
          {"destination:street:to", "Партизанский проспект"},
          {"destination:street:to:lang:ru", "Партизанский проспект"},
          {"destination:street:to:lang:be", "Партызанскі праспект"},
          {"destination:ref:to", "M4"}}},
        {"IK",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "М2"}}},
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Днепровская улица"},
          {"name:ru", "Днепровская улица"},
          {"name:be", "Дняпроўская вуліца"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "Гомель"},
          {"destination:street", "Днепровская улица"},
          {"destination:street:lang:ru", "Днепровская улица"},
          {"destination:street:lang:be", "Дняпроўская вуліца"}}},
    };

    const gurka::nodes nodes = {{"M", {{"highway", "traffic_signals"}, {"name", "Zaventem"}}}};

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {27.56191, 53.90246});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, nodes, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ru_be_MinskBelarus, CheckStreetNamesAndSigns1) {

  const std::string workdir = "test/data/gurka_language_with_streetname_and_sign_ru_be_MinskBelarus";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"A", "D"}, "auto");
  gurka::assert::raw::expect_path(result, {"М2", "", "МКАД, 1-й километр/1-ы кіламетр МКАД/M9"});

  // Verify starting on М2
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "М2");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets_size(),
            3);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(0)
                .text(),
            "M9");
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(1)
                .text(),
            "1-ы кіламетр МКАД");
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(2)
                .text(),
            "МКАД, 1-й километр");
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            2);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "Гомель");
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Слуцк");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "МКАД, 1-й километр");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "1-ы кіламетр МКАД");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "M9");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId BC_edge_id;
  const DirectedEdge* BC_edge = nullptr;
  GraphId CB_edge_id;
  const DirectedEdge* CB_edge = nullptr;
  std::tie(BC_edge_id, BC_edge, CB_edge_id, CB_edge) =
      findEdge(graph_reader, the_map.nodes, "", "C", baldr::GraphId{}, 101);
  EXPECT_NE(BC_edge, nullptr);
  EXPECT_NE(CB_edge, nullptr);

  GraphId JICDMN_edge_id;
  const DirectedEdge* JICDMN_edge = nullptr;
  GraphId NMDCIJ_edge_id;
  const DirectedEdge* NMDCIJ_edge = nullptr;
  std::tie(JICDMN_edge_id, JICDMN_edge, NMDCIJ_edge_id, NMDCIJ_edge) =
      findEdge(graph_reader, the_map.nodes, "", "N", baldr::GraphId{}, 100);
  EXPECT_NE(JICDMN_edge, nullptr);
  EXPECT_NE(NMDCIJ_edge, nullptr);

  GraphId node_id = BC_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(BC_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(BC_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 6);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "12");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "M9");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "1-ы кіламетр МКАД");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "be");

  ASSERT_EQ(edge_signs.at(3).text(), "МКАД, 1-й километр");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ru");

  ASSERT_EQ(edge_signs.at(4).text(), "Гомель");
  iter = linguistics.find(4);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(5).text(), "Слуцк");
  iter = linguistics.find(5);
  ASSERT_EQ(iter, linguistics.end());

  node_id = JICDMN_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(JICDMN_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();

  ASSERT_EQ(names_and_types.size(), 3);

  ASSERT_EQ(names_and_types.at(0).first, "МКАД, 1-й километр");
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ru");

  ASSERT_EQ(names_and_types.at(1).first, "1-ы кіламетр МКАД");
  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "be");

  ASSERT_EQ(names_and_types.at(2).first, "M9");
  lang_iter = languages.find(2);
  ASSERT_EQ(lang_iter, languages.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ru_be_MinskBelarus, CheckStreetNamesAndSigns2) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"G", "J"}, "auto");
  gurka::assert::raw::expect_path(result, {"М2", "", "МКАД, 1-й километр/1-ы кіламетр МКАД/M9"});

  // Verify starting on М2
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "М2");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            3);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "M4");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Партизанский проспект");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(2)
                .text(),
            "Партызанскі праспект");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId HI_edge_id;
  const DirectedEdge* HI_edge = nullptr;
  GraphId IH_edge_id;
  const DirectedEdge* IH_edge = nullptr;
  std::tie(HI_edge_id, HI_edge, IH_edge_id, IH_edge) =
      findEdge(graph_reader, the_map.nodes, "", "I", baldr::GraphId{}, 102);
  EXPECT_NE(HI_edge, nullptr);
  EXPECT_NE(IH_edge, nullptr);

  GraphId node_id = HI_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(HI_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(HI_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 4);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "12");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "M4");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Партизанский проспект");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ru");

  ASSERT_EQ(edge_signs.at(3).text(), "Партызанскі праспект");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "be");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ru_be_MinskBelarus, CheckGuideSigns) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"МКАД, 1-й километр/1-ы кіламетр МКАД/M9",
                                           "МКАД, 1-й километр/1-ы кіламетр МКАД/M9",
                                           "МКАД, 1-й километр/1-ы кіламетр МКАД/M9", "",
                                           "Днепровская улица/Дняпроўская вуліца"});

  // Verify starting on МКАД, 1-й километр/1-ы кіламетр МКАД/M9
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "МКАД, 1-й километр");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "1-ы кіламетр МКАД");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "M9");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets_size(),
            2);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(0)
                .text(),
            "Днепровская улица");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(1)
                .text(),
            "Дняпроўская вуліца");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations_size(),
            1);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations(0)
                .text(),
            "Гомель");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  GraphId node_id = PD_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(PD_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "Днепровская улица");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ru");

  ASSERT_EQ(edge_signs.at(1).text(), "Дняпроўская вуліца");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "be");

  ASSERT_EQ(edge_signs.at(2).text(), "Гомель");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ru_be_MinskBelarus, CheckNonJunctionName) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"МКАД, 1-й километр/1-ы кіламетр МКАД/M9",
                                           "МКАД, 1-й километр/1-ы кіламетр МКАД/M9",
                                           "МКАД, 1-й километр/1-ы кіламетр МКАД/M9",
                                           "МКАД, 1-й километр/1-ы кіламетр МКАД/M9",
                                           "Днепровская улица/Дняпроўская вуліца"});

  // Verify starting on МКАД, 1-й километр/1-ы кіламетр МКАД/M9
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "МКАД, 1-й километр");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "1-ы кіламетр МКАД");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "M9");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Днепровская улица");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ru");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Дняпроўская вуліца");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "be");

  // No junction should exist here.  Named junctions are not allowed in Belarus
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names_size(),
            0);
}

class RouteWithStreetnameAndSign_cy_en_Wales : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       J
                       |
                       |
                       |
                       I
                      /|\
                    /  |  \
                  /    |    \
           L----K-------------H----G
           A----B-------------E----F
                  \    |    /
                    \  |  /
                      \|/
                       C
                       |
                       |
                       |
                       D
               O------PM------Q
                       |
                       |
                       |
                       N

    )";

    const gurka::ways ways = {
        {"ABEF",
         {{"highway", "motorway"},
          {"name", "Gwibffordd Gogledd Cymru / North Wales Expressway"},
          {"name:en", "North Wales Expressway"},
          {"name:cy", "Gwibffordd Gogledd Cymru"},
          {"ref", "A55"},
          {"oneway", "yes"}}},
        {"GHKL",
         {{"highway", "motorway"},
          {"name", "Gwibffordd Gogledd Cymru / North Wales Expressway"},
          {"name:en", "North Wales Expressway"},
          {"name:cy", "Gwibffordd Gogledd Cymru"},
          {"ref", "A55"},
          {"oneway", "yes"}}},
        {"JICDMN",
         {{"highway", "primary"},
          {"osm_id", "100"},
          {"name", "Caernarfon Road"},
          {"name:cy", "Ffordd Caernarfon"},
          {"ref", "A4087"}}},
        {"BC",
         {{"highway", "motorway_link"},
          {"osm_id", "101"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination", "Newport;"},
          {"destination:lang:cy", "Casnewydd"},
          {"destination:street", "Caernarfon Road"},
          {"destination:street:lang:cy", "Ffordd Caernarfon"},
          {"destination:ref", "A4087"}}},
        {"CE",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "A55"}}},
        {"HI",
         {{"highway", "motorway_link"},
          {"osm_id", "102"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination:street:to", "Ainon Road"},
          {"destination:street:to:lang:cy", "Ffordd Ainion"},
          {"destination:ref:to", "M4"}}},
        {"IK",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "A55"}}},
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "North Street"},
          {"name:en", "Penchwintan Road"},
          {"name:cy", "Ffordd Penchwintan"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "Newport"},
          {"destination:street", "Penchwintan Road"},
          {"destination:street:lang:cy", "Ffordd Penchwintan"}}},
    };

    const gurka::nodes nodes = {{"M", {{"highway", "traffic_signals"}, {"name", "M Junction"}}}};

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {-3.73895, 52.29282});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, nodes, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_cy_en_Wales, CheckStreetNamesAndSigns1) {

  const std::string workdir = "test/data/gurka_language_with_streetname_and_sign_cy_en_Wales";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"A", "D"}, "auto");
  gurka::assert::raw::expect_path(result, {"A55/Gwibffordd Gogledd Cymru/North Wales Expressway", "",
                                           "Caernarfon Road/Ffordd Caernarfon/A4087"});

  // Verify starting on A55
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "A55");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Gwibffordd Gogledd Cymru");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "North Wales Expressway");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets_size(),
            3);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(0)
                .text(),
            "A4087");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(1)
                .text(),
            "Caernarfon Road");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(2)
                .text(),
            "Ffordd Caernarfon");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            2);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "Newport");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Casnewydd");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Caernarfon Road");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Ffordd Caernarfon");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "A4087");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId BC_edge_id;
  const DirectedEdge* BC_edge = nullptr;
  GraphId CB_edge_id;
  const DirectedEdge* CB_edge = nullptr;
  std::tie(BC_edge_id, BC_edge, CB_edge_id, CB_edge) =
      findEdge(graph_reader, the_map.nodes, "", "C", baldr::GraphId{}, 101);
  EXPECT_NE(BC_edge, nullptr);
  EXPECT_NE(CB_edge, nullptr);

  GraphId JICDMN_edge_id;
  const DirectedEdge* JICDMN_edge = nullptr;
  GraphId NMDCIJ_edge_id;
  const DirectedEdge* NMDCIJ_edge = nullptr;
  std::tie(JICDMN_edge_id, JICDMN_edge, NMDCIJ_edge_id, NMDCIJ_edge) =
      findEdge(graph_reader, the_map.nodes, "", "N", baldr::GraphId{}, 100);
  EXPECT_NE(JICDMN_edge, nullptr);
  EXPECT_NE(NMDCIJ_edge, nullptr);

  GraphId node_id = BC_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(BC_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(BC_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 6);
  ASSERT_EQ(linguistics.size(), 4);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "A4087");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Caernarfon Road");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(3).text(), "Ffordd Caernarfon");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "cy");

  ASSERT_EQ(edge_signs.at(4).text(), "Newport");
  iter = linguistics.find(4);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(5).text(), "Casnewydd");
  iter = linguistics.find(5);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "cy");

  node_id = JICDMN_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(JICDMN_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();

  ASSERT_EQ(names_and_types.size(), 3);

  ASSERT_EQ(names_and_types.at(0).first, "Caernarfon Road");
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  ASSERT_EQ(names_and_types.at(1).first, "Ffordd Caernarfon");
  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "cy");

  ASSERT_EQ(names_and_types.at(2).first, "A4087");
  lang_iter = languages.find(2);
  ASSERT_EQ(lang_iter, languages.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_cy_en_Wales, CheckStreetNamesAndSigns2) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"G", "J"}, "auto");
  gurka::assert::raw::expect_path(result, {"A55/Gwibffordd Gogledd Cymru/North Wales Expressway", "",
                                           "Caernarfon Road/Ffordd Caernarfon/A4087"});

  // Verify starting on A55
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "A55");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Gwibffordd Gogledd Cymru");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "North Wales Expressway");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            3);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "M4");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Ainon Road");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(2)
                .text(),
            "Ffordd Ainion");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId HI_edge_id;
  const DirectedEdge* HI_edge = nullptr;
  GraphId IH_edge_id;
  const DirectedEdge* IH_edge = nullptr;
  std::tie(HI_edge_id, HI_edge, IH_edge_id, IH_edge) =
      findEdge(graph_reader, the_map.nodes, "", "I", baldr::GraphId{}, 102);
  EXPECT_NE(HI_edge, nullptr);
  EXPECT_NE(IH_edge, nullptr);

  GraphId node_id = HI_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(HI_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(HI_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 4);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "M4");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Ainon Road");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(3).text(), "Ffordd Ainion");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "cy");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_cy_en_Wales, CheckGuideSigns) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Caernarfon Road/Ffordd Caernarfon/A4087",
                                           "Caernarfon Road/Ffordd Caernarfon/A4087",
                                           "Caernarfon Road/Ffordd Caernarfon/A4087", "",
                                           "North Street/Ffordd Penchwintan/Penchwintan Road"});

  // Verify starting on Caernarfon Road/A4087
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Caernarfon Road");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Ffordd Caernarfon");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "A4087");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets_size(),
            2);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(0)
                .text(),
            "Penchwintan Road");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(1)
                .text(),
            "Ffordd Penchwintan");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations_size(),
            1);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations(0)
                .text(),
            "Newport");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  GraphId node_id = PD_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(PD_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "Penchwintan Road");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(1).text(), "Ffordd Penchwintan");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "cy");

  ASSERT_EQ(edge_signs.at(2).text(), "Newport");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_cy_en_Wales, CheckNonJunctionName) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Caernarfon Road/Ffordd Caernarfon/A4087",
                                           "Caernarfon Road/Ffordd Caernarfon/A4087",
                                           "Caernarfon Road/Ffordd Caernarfon/A4087",
                                           "Caernarfon Road/Ffordd Caernarfon/A4087",
                                           "North Street/Ffordd Penchwintan/Penchwintan Road"});

  // Verify starting on Caernarfon Road/A4087
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Caernarfon Road");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Ffordd Caernarfon");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "A4087");

  // Verify street name language tag
  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "North Street");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Ffordd Penchwintan");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Penchwintan Road");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 3);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "North Street");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Ffordd Penchwintan");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "cy");

  lang_iter = languages.find(2);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(2).first, "Penchwintan Road");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  // No junction should exist here.  Named junctions are not allowed in UK
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names_size(),
            0);
}

class RouteWithStreetnameAndSign_fr_nl_BrusselsBelgiumRightLeft : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Chaussée de Gand - Steenweg op Gent/Gentsesteenweg"},
          {"name:fr", "Chaussée de Gand"},
          {"name:left", "Chaussée de Gand - Gentsesteenweg"},
          {"name:left:nl", "Gentsesteenweg"},
          {"name:right", "Chaussée de Gand - Steenweg op Gent"},
          {"name:right:nl", "Steenweg op Gent"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {4.3516970, 50.8465573});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_BrusselsBelgiumRightLeft, CheckRightNames) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_fr_nl_BrusselsBelgiumRightLeft";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Chaussée de Gand/Steenweg op Gent"});

  // Verify starting on Chaussée de Gand/Steenweg op Gent
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Chaussée de Gand");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Steenweg op Gent");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Chaussée de Gand");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Steenweg op Gent");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "nl");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_BrusselsBelgiumRightLeft, CheckLeftNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Chaussée de Gand/Gentsesteenweg"});

  // Verify starting on Chaussée de Gand/Gentsesteenweg
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Chaussée de Gand");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Gentsesteenweg");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Chaussée de Gand");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Gentsesteenweg");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "nl");
}

class RouteWithStreetnameAndSign_en_USForwardBackwardWithName : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Waltonville Road"},
          {"name:forward", "Waltonville Road"},
          {"name:backward", "Quarry Road"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {-76.69980, 40.25882});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_USForwardBackwardWithName, CheckForwardNames) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_en_USForwardBackwardwithName";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Waltonville Road"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Waltonville Road");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_USForwardBackwardWithName, CheckBackwardNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Quarry Road"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Quarry Road");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}

class RouteWithStreetnameAndSign_en_USForwardBackwardNoName : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", ""},
          {"name:forward", "Waltonville Road"},
          {"name:backward", "Quarry Road"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {-76.69980, 40.25882});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_USForwardBackwardNoName, CheckForwardNames) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_en_USForwardBackwardwithName";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Waltonville Road"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Waltonville Road");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_USForwardBackwardNoName, CheckBackwardNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Quarry Road"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Quarry Road");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}

/* TODO  Fix this edge case. This works while using degrees (boost) for points but it is too slow.
class RouteWithStreetnameAndSign_fr_nl_MesenBelgiumRightLeft : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"QMPO",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Komenstraat - Chemin des Quatre Rois"},
          {"name:left", "Chemin des Quatre Rois"},
          {"name:left:fr", "Chemin des Quatre Rois"},
          {"name:left:nl", "Vier Koningenweg"},
          {"name:right", "Komenstraat"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {2.9305333, 50.7672572});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_MesenBelgiumRightLeft, CheckRightNames) {

  // Supports name:left when traveling O to Q.  Supports name:right when traveling Q to O
  // See https://www.openstreetmap.org/relation/90348#map=15/50.7660/2.9469 and
  // https://www.openstreetmap.org/way/30126046#map=17/50.76759/2.93026 note the way has been flipped.
  // starts at O and ends at Q Bail on language as the way left and right and name info is not 100%
  // correct and we are traversing on the polygon perimeters

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_fr_nl_MesenBelgiumRightLeft";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Komenstraat"});

  // Verify starting on Komenstraat
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Komenstraat");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Komenstraat");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_MesenBelgiumRightLeft, CheckLeftNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Chemin des Quatre Rois"});

  // Verify starting on Chaussée de Gand/Steenweg op Gent
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 1);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Chemin des Quatre Rois");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Chemin des Quatre Rois");
}
*/

class RouteWithStreetnameAndSign_fr_de_FribourgSwitzerlandMulti : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"QMPO",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Route des Alpes / Alpenstrasse"},
          {"name:de", "Alpenstrasse"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {7.159328, 46.805244});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_de_FribourgSwitzerlandMulti, CheckForwardNames) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_fr_de_FribourgSwitzerlandMulti";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Route des Alpes/Alpenstrasse"});
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");
  ASSERT_EQ(names_and_types.at(0).first, "Route des Alpes");

  lang_iter = languages.find(1);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "de");
  ASSERT_EQ(names_and_types.at(1).first, "Alpenstrasse");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_de_FribourgSwitzerlandMulti, CheckBackwardNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Route des Alpes/Alpenstrasse"});
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");
  ASSERT_EQ(names_and_types.at(0).first, "Route des Alpes");

  lang_iter = languages.find(1);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "de");
  ASSERT_EQ(names_and_types.at(1).first, "Alpenstrasse");
}

class RouteWithStreetnameAndSign_rm_de_BivioSwitzerland : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Vea del Giulia"},
          {"name:it", "Vea del Giulia"},
          {"name:de", "Julierstrasse"},
          {"name:rm", "Via digl Gelgia"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {9.65035, 46.46977});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_rm_de_BivioSwitzerland, CheckForwardNames) {

  // language IT should be dropped, but since in name tag we don't toss.
  // Lingustic polys/default languages in this area is rm and de.
  // actual hotel address:
  // Hotel Post, Julierstrasse 64 CH-7457 Bivio
  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_rm_de_BivioSwitzerland";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Vea del Giulia/Via digl Gelgia/Julierstrasse"});
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 3);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Vea del Giulia");

  lang_iter = languages.find(1);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "rm");
  ASSERT_EQ(names_and_types.at(1).first, "Via digl Gelgia");

  lang_iter = languages.find(2);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "de");
  ASSERT_EQ(names_and_types.at(2).first, "Julierstrasse");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_rm_de_BivioSwitzerland, CheckBackwardNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Vea del Giulia/Via digl Gelgia/Julierstrasse"});
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 3);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Vea del Giulia");

  lang_iter = languages.find(1);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "rm");
  ASSERT_EQ(names_and_types.at(1).first, "Via digl Gelgia");

  lang_iter = languages.find(2);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "de");
  ASSERT_EQ(names_and_types.at(2).first, "Julierstrasse");
}

class RouteWithStreetnameAndSign_de_ZurichSwitzerland : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"OPMQ", {{"highway", "secondary"}, {"osm_id", "103"}, {"name", "Werdstrasse"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {8.5355, 47.3726});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_de_ZurichSwitzerland, CheckForwardNames) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_de_ZurichSwitzerland";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Werdstrasse"});
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "de");
  ASSERT_EQ(names_and_types.at(0).first, "Werdstrasse");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_de_ZurichSwitzerland, CheckBackwardNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Werdstrasse"});
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 1);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "de");
  ASSERT_EQ(names_and_types.at(0).first, "Werdstrasse");
}

class RouteWithStreetnameAndSign_fr_nl_EupenBelgium : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       D
               O------PM------Q

    )";

    const gurka::ways ways = {
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Bergstraße"},
          {"name:de", "Bergstraße"},
          {"name:fr", "Rue de la Montagne"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "Eupen"},
          {"destination:street", "Bergstraße"},
          {"destination:street:lang:fr", "Rue de la Montagne"},
          {"destination:street:lang:nl", "Lebeaustraat"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {6.03475, 50.62766});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_fr_nl_EupenBelgium, CheckLingusticPoly) {

  const std::string workdir = "test/data/gurka_language_with_streetname_and_sign_fr_nl_EupenBelgium";

  // Supports DE and FR.  DE is a higher priority as it is a German community
  // See https://www.openstreetmap.org/relation/2425209#map=10/50.4440/6.1805 and
  // https://www.openstreetmap.org/relation/90348

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result,
                                  {"Bergstraße/Rue de la Montagne", "Bergstraße/Rue de la Montagne"});

  // Verify starting on Chaussée de Gand/Steenweg op Gent
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Bergstraße");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Rue de la Montagne");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Bergstraße");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "de");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Rue de la Montagne");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  node_id = PD_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(PD_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "Bergstraße");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "de");

  ASSERT_EQ(edge_signs.at(1).text(), "Rue de la Montagne");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(2).text(), "Eupen");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());
}

class RouteWithStreetnameAndSign_ja_en_Japan : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       J
                       |
                       |
                       |
                       I
                      /|\
                    /  |  \
                  /    |    \
           L----K-------------H----G
           A----B-------------E----F
                  \    |    /
                    \  |  /
                      \|/
                       C
                       |
                       |
                       |
                       D
               O------PM------Q
                       |
                       |
                       |
                       N

    )";

    const gurka::ways ways = {
        {"ABEF",
         {{"highway", "motorway"},
          {"name", "首都高速6号向島線"},
          {"name:en", "Shuto Expressway Route 6 Mukojima Line"},
          {"name:es", "Ruta 6 Mukojima de la Autopista Shuto"},
          {"name:ja", "首都高速6号向島線"},
          {"name:ru", "Шоссе Мукодзима"},
          {"ref", "6"},
          {"oneway", "yes"}}},
        {"GHKL",
         {{"highway", "motorway"},
          {"name", "首都高速6号向島線"},
          {"name:en", "Shuto Expressway Route 6 Mukojima Line"},
          {"name:es", "Ruta 6 Mukojima de la Autopista Shuto"},
          {"name:ja", "首都高速6号向島線"},
          {"name:ru", "Шоссе Мукодзима"},
          {"ref", "6"},
          {"oneway", "yes"}}},
        {"JICDMN",
         {{"highway", "primary"},
          {"osm_id", "100"},
          {"name", "常磐道;東北道"},
          {"name:en", "Joban Expressway;Tohoku Expressway"},
          {"ref", "E6;E4"}}},
        {"BC",
         {{"highway", "motorway_link"},
          {"osm_id", "101"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination", "常磐道;東北道;"},
          {"destination:lang:en", "Joban Expressway;Tohoku Expressway"},
          {"destination:street", "清洲橋通り"},
          {"destination:street:lang:en", "Kiyosubashi-dori Avenue"},
          {"destination:street:lang:es", "Calle Kiyosubashi"},
          {"destination:street:lang:ja", "清洲橋通り"},
          {"destination:street:lang:ja_rm", "Kiyosubashi Dōri"},
          {"destination:ref", "E6;E4"}}},
        {"CE",
         {{"highway", "motorway_link"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination:ref", "E6;E4"}}},
        {"HI",
         {{"highway", "motorway_link"},
          {"osm_id", "102"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination:street:to", "清澄通り"},
          {"destination:street:to:lang:en", "Kiyosumi-dori"},
          {"destination:street:to:lang:es", "Calle Kiyosumi"},
          {"destination:street:to:lang:ja_kana", "きよすみどおり"},
          {"destination:ref:to", "M4"}}},
        {"IK",
         {{"highway", "motorway_link"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination:ref", "E6;E4"}}},
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "国技館通り"},
          {"name:en", "Kokugikan-dori"},
          {"name:es", "Calle Kokugikan"},
          {"name:ja_kana", "こくぎかんどおり"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "大和街道"},
          {"destination:street", "国技館通り"},
          {"destination:street:lang:en", "Kokugikan-dori"},
          {"destination:street:lang:es", "Calle Kokugikan"},
          {"destination:street:lang:ja_kana", "こくぎかんどおり"}}},
    };

    const gurka::nodes nodes = {
        {"M",
         {{"highway", "traffic_signals"}, {"name", "両国二丁目"}, {"name:en", "Ryogoku 2-chome"}}}};

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {139.79079, 35.69194});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, nodes, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ja_en_Japan, CheckStreetNamesAndSigns1) {

  const std::string workdir = "test/data/gurka_language_with_streetname_and_sign_ja_en_Japan";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"A", "D"}, "auto");
  gurka::assert::raw::expect_path(result,
                                  {"6/首都高速6号向島線/Shuto Expressway Route 6 Mukojima Line", "",
                                   "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4"});

  // Verify starting on 6
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "6");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "首都高速6号向島線");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Shuto Expressway Route 6 Mukojima Line");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets_size(),
            4);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(0)
                .text(),
            "E6");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(1)
                .text(),
            "E4");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(2)
                .text(),
            "清洲橋通り");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(3)
                .text(),
            "Kiyosubashi-dori Avenue");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            4);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "常磐道");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "東北道");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(2)
                .text(),
            "Joban Expressway");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(3)
                .text(),
            "Tohoku Expressway");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 6);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "常磐道");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "東北道");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Joban Expressway");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(3).value(),
            "Tohoku Expressway");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(4).value(),
            "E6");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(5).value(),
            "E4");
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId BC_edge_id;
  const DirectedEdge* BC_edge = nullptr;
  GraphId CB_edge_id;
  const DirectedEdge* CB_edge = nullptr;
  std::tie(BC_edge_id, BC_edge, CB_edge_id, CB_edge) =
      findEdge(graph_reader, the_map.nodes, "", "C", baldr::GraphId{}, 101);
  EXPECT_NE(BC_edge, nullptr);
  EXPECT_NE(CB_edge, nullptr);

  GraphId JICDMN_edge_id;
  const DirectedEdge* JICDMN_edge = nullptr;
  GraphId NMDCIJ_edge_id;
  const DirectedEdge* NMDCIJ_edge = nullptr;
  std::tie(JICDMN_edge_id, JICDMN_edge, NMDCIJ_edge_id, NMDCIJ_edge) =
      findEdge(graph_reader, the_map.nodes, "", "N", baldr::GraphId{}, 100);
  EXPECT_NE(JICDMN_edge, nullptr);
  EXPECT_NE(NMDCIJ_edge, nullptr);

  GraphId node_id = BC_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(BC_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(BC_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 9);
  ASSERT_EQ(linguistics.size(), 6);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "E6");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "E4");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(3).text(), "清洲橋通り");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(4).text(), "Kiyosubashi-dori Avenue");
  iter = linguistics.find(4);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(5).text(), "常磐道");
  iter = linguistics.find(5);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(6).text(), "東北道");
  iter = linguistics.find(6);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(7).text(), "Joban Expressway");
  iter = linguistics.find(7);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(8).text(), "Tohoku Expressway");
  iter = linguistics.find(8);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  node_id = JICDMN_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(JICDMN_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();

  ASSERT_EQ(names_and_types.size(), 6);

  ASSERT_EQ(names_and_types.at(0).first, "常磐道");
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ja");

  ASSERT_EQ(names_and_types.at(1).first, "東北道");
  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ja");

  ASSERT_EQ(names_and_types.at(2).first, "Joban Expressway");
  lang_iter = languages.find(2);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  ASSERT_EQ(names_and_types.at(3).first, "Tohoku Expressway");
  lang_iter = languages.find(3);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  ASSERT_EQ(names_and_types.at(4).first, "E6");
  lang_iter = languages.find(4);
  ASSERT_EQ(lang_iter, languages.end());

  ASSERT_EQ(names_and_types.at(5).first, "E4");
  lang_iter = languages.find(5);
  ASSERT_EQ(lang_iter, languages.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ja_en_Japan, CheckStreetNamesAndSigns2) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"G", "J"}, "auto");
  gurka::assert::raw::expect_path(result,
                                  {"6/首都高速6号向島線/Shuto Expressway Route 6 Mukojima Line", "",
                                   "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4"});

  // Verify starting on 6
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "6");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "首都高速6号向島線");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Shuto Expressway Route 6 Mukojima Line");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            3);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "M4");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "清澄通り");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(2)
                .text(),
            "Kiyosumi-dori");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId HI_edge_id;
  const DirectedEdge* HI_edge = nullptr;
  GraphId IH_edge_id;
  const DirectedEdge* IH_edge = nullptr;
  std::tie(HI_edge_id, HI_edge, IH_edge_id, IH_edge) =
      findEdge(graph_reader, the_map.nodes, "", "I", baldr::GraphId{}, 102);
  EXPECT_NE(HI_edge, nullptr);
  EXPECT_NE(IH_edge, nullptr);

  GraphId node_id = HI_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(HI_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(HI_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 4);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "M4");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "清澄通り");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(3).text(), "Kiyosumi-dori");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ja_en_Japan, CheckGuideSigns) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "", "国技館通り/Kokugikan-dori"});

  // Verify starting on 常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 6);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "常磐道");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "東北道");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Joban Expressway");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(3).value(),
            "Tohoku Expressway");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(4).value(),
            "E6");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(5).value(),
            "E4");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets_size(),
            2);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(0)
                .text(),
            "国技館通り");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(1)
                .text(),
            "Kokugikan-dori");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations_size(),
            1);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations(0)
                .text(),
            "大和街道");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  GraphId node_id = PD_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(PD_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "国技館通り");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(1).text(), "Kokugikan-dori");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(2).text(), "大和街道");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ja_en_Japan, CheckNonJunctionName) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "国技館通り/Kokugikan-dori"});

  // Verify starting on 常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 6);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "常磐道");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "東北道");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Joban Expressway");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(3).value(),
            "Tohoku Expressway");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(4).value(),
            "E6");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(5).value(),
            "E4");

  // Verify street name language tag
  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "国技館通り");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Kokugikan-dori");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "国技館通り");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ja");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Kokugikan-dori");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  // Junction should exist here.  Named junctions are allowed in JP
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names_size(),
            2);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names(0)
                .text(),
            "両国二丁目");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names(1)
                .text(),
            "Ryogoku 2-chome");

  GraphId DM_edge_id;
  const DirectedEdge* DM_edge = nullptr;
  GraphId MD_edge_id;
  const DirectedEdge* MD_edge = nullptr;
  std::tie(DM_edge_id, DM_edge, MD_edge_id, MD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "M", baldr::GraphId{}, 100);
  EXPECT_NE(DM_edge, nullptr);
  EXPECT_NE(MD_edge, nullptr);

  node_id = DM_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(node_id.id(), linguistics, true);

  ASSERT_EQ(edge_signs.size(), 2);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "両国二丁目");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(1).text(), "Ryogoku 2-chome");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");
}

class RouteWithStreetnameAndSign_en_fr_OttawaCanada : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       J
                       |
                       |
                       |
                       I
                      /|\
                    /  |  \
                  /    |    \
           L----K-------------H----G
           A----B-------------E----F
                  \    |    /
                    \  |  /
                      \|/
                       C
                       |
                       |
                       |
                       D
               O------PM------Q
                       |
                       |
                       |
                       N

    )";

    const gurka::ways ways = {
        {"ABEF",
         {{"highway", "motorway"},
          {"name", "Highway 417"},
          {"name:en", "Highway 417"},
          {"name:fr", "Route 417"},
          {"ref", "417"},
          {"nat_name", "Trans-Canada Highway"},
          {"nat_name:en", "Trans-Canada Highway"},
          {"nat_name:fr", "   Route Transcanadienne"},
          {"oneway", "yes"}}},
        {"GHKL",
         {{"highway", "motorway"},
          {"name", "Highway 417"},
          {"name:en", "Highway 417"},
          {"name:fr", "Route 417"},
          {"ref", "417"},
          {"nat_name", "Trans-Canada Highway"},
          {"nat_name:en", "Trans-Canada Highway"},
          {"nat_name:fr", "   Route Transcanadienne"},
          {"oneway", "yes"}}},
        {"JICDMN",
         {{"highway", "primary"},
          {"osm_id", "100"},
          {"name", ""},
          {"name:en", "Vanier Parkway"},
          {"name:fr", "promenade Vanier"},
          {"ref", "19"}}},
        {"BC",
         {{"highway", "motorway_link"},
          {"osm_id", "101"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination", "Sandy Hill;"},
          {"destination:lang:fr", "Colline de sable"},
          {"destination:street", "Vanier Parkway;Riverside Drive"},
          {"destination:street:lang:fr", "Promenade Vanier;Promenade Riverside"},
          {"destination:street:lang:en", "Vanier Parkway;Riverside Drive"},
          {"destination:ref", "19"}}},
        {"CE",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "417"}}},
        {"HI",
         {{"highway", "motorway_link"},
          {"osm_id", "102"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination:street:to", "Queen Street"},
          {"destination:street:to:lang:fr", "rue Queen"},
          {"destination:ref:to", "19"}}},
        {"IK",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "417"}}},
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Albert Street"},
          {"name:en", "Albert Street"},
          {"name:fr", "rue Albert"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "Centretown"},
          {"destination:street", "rue Albert"},
          {"destination:street:lang:en", "Albert Street"}}},
    };

    const gurka::nodes nodes = {{"M", {{"highway", "traffic_signals"}, {"name", "M Junction"}}}};

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {-75.6625, 45.3940});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, nodes, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_fr_OttawaCanada, CheckStreetNamesAndSigns1) {

  const std::string workdir = "test/data/gurka_language_with_streetname_and_sign_en_fr_OttawaCanada";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"A", "D"}, "auto");
  gurka::assert::raw::expect_path(result, {"417/Highway 417/Route 417", "",
                                           "Vanier Parkway/promenade Vanier/19"});

  // Verify starting on 417
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "417");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Highway 417");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Route 417");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets_size(),
            5);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(0)
                .text(),
            "19");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(1)
                .text(),
            "Vanier Parkway");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(2)
                .text(),
            "Riverside Drive");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(3)
                .text(),
            "Promenade Vanier");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(4)
                .text(),
            "Promenade Riverside");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            2);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "Sandy Hill");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Colline de sable");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Vanier Parkway");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "promenade Vanier");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "19");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId BC_edge_id;
  const DirectedEdge* BC_edge = nullptr;
  GraphId CB_edge_id;
  const DirectedEdge* CB_edge = nullptr;
  std::tie(BC_edge_id, BC_edge, CB_edge_id, CB_edge) =
      findEdge(graph_reader, the_map.nodes, "", "C", baldr::GraphId{}, 101);
  EXPECT_NE(BC_edge, nullptr);
  EXPECT_NE(CB_edge, nullptr);

  GraphId JICDMN_edge_id;
  const DirectedEdge* JICDMN_edge = nullptr;
  GraphId NMDCIJ_edge_id;
  const DirectedEdge* NMDCIJ_edge = nullptr;
  std::tie(JICDMN_edge_id, JICDMN_edge, NMDCIJ_edge_id, NMDCIJ_edge) =
      findEdge(graph_reader, the_map.nodes, "", "N", baldr::GraphId{}, 100);
  EXPECT_NE(JICDMN_edge, nullptr);
  EXPECT_NE(NMDCIJ_edge, nullptr);

  GraphId node_id = BC_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(BC_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(BC_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 8);
  ASSERT_EQ(linguistics.size(), 6);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "19");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Vanier Parkway");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(3).text(), "Riverside Drive");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(4).text(), "Promenade Vanier");
  iter = linguistics.find(4);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(5).text(), "Promenade Riverside");
  iter = linguistics.find(5);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(6).text(), "Sandy Hill");
  iter = linguistics.find(6);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(7).text(), "Colline de sable");
  iter = linguistics.find(7);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  node_id = JICDMN_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(JICDMN_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();

  ASSERT_EQ(names_and_types.size(), 3);

  ASSERT_EQ(names_and_types.at(0).first, "Vanier Parkway");
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  ASSERT_EQ(names_and_types.at(1).first, "promenade Vanier");
  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  ASSERT_EQ(names_and_types.at(2).first, "19");
  lang_iter = languages.find(2);
  ASSERT_EQ(lang_iter, languages.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_fr_OttawaCanada, CheckStreetNamesAndSigns2) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"G", "J"}, "auto");
  gurka::assert::raw::expect_path(result, {"417/Highway 417/Route 417", "",
                                           "Vanier Parkway/promenade Vanier/19"});

  // Verify starting on 417
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "417");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Highway 417");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Route 417");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            3);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "19");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Queen Street");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(2)
                .text(),
            "rue Queen");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId HI_edge_id;
  const DirectedEdge* HI_edge = nullptr;
  GraphId IH_edge_id;
  const DirectedEdge* IH_edge = nullptr;
  std::tie(HI_edge_id, HI_edge, IH_edge_id, IH_edge) =
      findEdge(graph_reader, the_map.nodes, "", "I", baldr::GraphId{}, 102);
  EXPECT_NE(HI_edge, nullptr);
  EXPECT_NE(IH_edge, nullptr);

  GraphId node_id = HI_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(HI_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(HI_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 4);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "19");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Queen Street");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(3).text(), "rue Queen");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_fr_OttawaCanada, CheckGuideSigns) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Vanier Parkway/promenade Vanier/19",
                                           "Vanier Parkway/promenade Vanier/19",
                                           "Vanier Parkway/promenade Vanier/19", "",
                                           "Albert Street/rue Albert"});

  // Verify starting on Vanier Parkway/promenade Vanier/19
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Vanier Parkway");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "promenade Vanier");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "19");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets_size(),
            2);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(0)
                .text(),
            "rue Albert");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(1)
                .text(),
            "Albert Street");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations_size(),
            1);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations(0)
                .text(),
            "Centretown");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  GraphId node_id = PD_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(PD_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 2);

  // note flipped on purpose
  ASSERT_EQ(edge_signs.at(0).text(), "rue Albert");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(1).text(), "Albert Street");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(2).text(), "Centretown");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_fr_OttawaCanada, CheckNonJunctionName) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "Q"}, "auto");
  gurka::assert::raw::expect_path(result,
                                  {"Vanier Parkway/promenade Vanier/19",
                                   "Vanier Parkway/promenade Vanier/19",
                                   "Vanier Parkway/promenade Vanier/19",
                                   "Vanier Parkway/promenade Vanier/19", "Albert Street/rue Albert"});

  // Verify starting on Vanier Parkway/promenade Vanier/19
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Vanier Parkway");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "promenade Vanier");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "19");

  // Verify street name language tag
  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Albert Street");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "rue Albert");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);

  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Albert Street");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "rue Albert");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  // No junction should exist here.  Named junctions are not allowed in CA
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names_size(),
            0);
}

class RouteWithStreetnameAndSign_en_fr_QuebecCanada : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       J
                       |
                       |
                       |
                       I
                      /|\
                    /  |  \
                  /    |    \
           L----K-------------H----G
           A----B-------------E----F
                  \    |    /
                    \  |  /
                      \|/
                       C
                       |
                       |
                       |
                       D
               O------PM------Q
                       |
                       |
                       |
                       N

    )";

    const gurka::ways ways = {
        {"ABEF",
         {{"highway", "motorway"},
          {"name", "Highway 417"},
          {"name:en", "Highway 417"},
          {"name:fr", "Route 417"},
          {"ref", "417"},
          {"nat_name", "Trans-Canada Highway"},
          {"nat_name:en", "Trans-Canada Highway"},
          {"nat_name:fr", "   Route Transcanadienne"},
          {"oneway", "yes"}}},
        {"GHKL",
         {{"highway", "motorway"},
          {"name", "Highway 417"},
          {"name:en", "Highway 417"},
          {"name:fr", "Route 417"},
          {"ref", "417"},
          {"nat_name", "Trans-Canada Highway"},
          {"nat_name:en", "Trans-Canada Highway"},
          {"nat_name:fr", "   Route Transcanadienne"},
          {"oneway", "yes"}}},
        {"JICDMN",
         {{"highway", "primary"},
          {"osm_id", "100"},
          {"name", ""},
          {"name:en", "Vanier Parkway"},
          {"name:fr", "promenade Vanier"},
          {"ref", "19"}}},
        {"BC",
         {{"highway", "motorway_link"},
          {"osm_id", "101"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination", "Sandy Hill;"},
          {"destination:lang:fr", "Colline de sable"},
          {"destination:street", "Vanier Parkway;Riverside Drive"},
          {"destination:street:lang:fr", "Promenade Vanier;Promenade Riverside"},
          {"destination:street:lang:en", "Vanier Parkway;Riverside Drive"},
          {"destination:ref", "19"}}},
        {"CE",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "417"}}},
        {"HI",
         {{"highway", "motorway_link"},
          {"osm_id", "102"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination:street:to", "Queen Street"},
          {"destination:street:to:lang:fr", "rue Queen"},
          {"destination:ref:to", "19"}}},
        {"IK",
         {{"highway", "motorway_link"}, {"name", ""}, {"oneway", "yes"}, {"destination:ref", "417"}}},
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Albert Street"},
          {"name:en", "Albert Street"},
          {"name:fr", "rue Albert"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "Centretown"},
          {"destination:street", "rue Albert"},
          {"destination:street:lang:en", "Albert Street"}}},
    };

    const gurka::nodes nodes = {{"M", {{"highway", "traffic_signals"}, {"name", "M Junction"}}}};

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {-71.2593, 46.8111});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, nodes, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_fr_QuebecCanada, CheckStreetNamesAndSigns1) {

  const std::string workdir = "test/data/gurka_language_with_streetname_and_sign_en_fr_QuebecCanada";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"A", "D"}, "auto");
  gurka::assert::raw::expect_path(result, {"417/Highway 417/Route 417", "",
                                           "promenade Vanier/Vanier Parkway/19"});

  // Verify starting on 417
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "417");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Highway 417");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Route 417");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets_size(),
            5);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(0)
                .text(),
            "19");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(1)
                .text(),
            "Vanier Parkway");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(2)
                .text(),
            "Riverside Drive");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(3)
                .text(),
            "Promenade Vanier");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_onto_streets(4)
                .text(),
            "Promenade Riverside");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            2);
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "Sandy Hill");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Colline de sable");

  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "promenade Vanier");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Vanier Parkway");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "19");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId BC_edge_id;
  const DirectedEdge* BC_edge = nullptr;
  GraphId CB_edge_id;
  const DirectedEdge* CB_edge = nullptr;
  std::tie(BC_edge_id, BC_edge, CB_edge_id, CB_edge) =
      findEdge(graph_reader, the_map.nodes, "", "C", baldr::GraphId{}, 101);
  EXPECT_NE(BC_edge, nullptr);
  EXPECT_NE(CB_edge, nullptr);

  GraphId JICDMN_edge_id;
  const DirectedEdge* JICDMN_edge = nullptr;
  GraphId NMDCIJ_edge_id;
  const DirectedEdge* NMDCIJ_edge = nullptr;
  std::tie(JICDMN_edge_id, JICDMN_edge, NMDCIJ_edge_id, NMDCIJ_edge) =
      findEdge(graph_reader, the_map.nodes, "", "N", baldr::GraphId{}, 100);
  EXPECT_NE(JICDMN_edge, nullptr);
  EXPECT_NE(NMDCIJ_edge, nullptr);

  GraphId node_id = BC_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(BC_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(BC_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 8);
  ASSERT_EQ(linguistics.size(), 6);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "19");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Vanier Parkway");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(3).text(), "Riverside Drive");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(4).text(), "Promenade Vanier");
  iter = linguistics.find(4);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(5).text(), "Promenade Riverside");
  iter = linguistics.find(5);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(6).text(), "Sandy Hill");
  iter = linguistics.find(6);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(7).text(), "Colline de sable");
  iter = linguistics.find(7);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  node_id = JICDMN_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(JICDMN_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();

  ASSERT_EQ(names_and_types.size(), 3);

  ASSERT_EQ(names_and_types.at(0).first, "promenade Vanier");
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  ASSERT_EQ(names_and_types.at(1).first, "Vanier Parkway");
  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  ASSERT_EQ(names_and_types.at(2).first, "19");
  lang_iter = languages.find(2);
  ASSERT_EQ(lang_iter, languages.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_fr_QuebecCanada, CheckStreetNamesAndSigns2) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"G", "J"}, "auto");
  gurka::assert::raw::expect_path(result, {"417/Highway 417/Route 417", "",
                                           "promenade Vanier/Vanier Parkway/19"});

  // Verify starting on 417
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "417");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Highway 417");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "Route 417");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations_size(),
            3);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(0)
                .text(),
            "19");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(1)
                .text(),
            "Queen Street");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .exit_toward_locations(2)
                .text(),
            "rue Queen");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId HI_edge_id;
  const DirectedEdge* HI_edge = nullptr;
  GraphId IH_edge_id;
  const DirectedEdge* IH_edge = nullptr;
  std::tie(HI_edge_id, HI_edge, IH_edge_id, IH_edge) =
      findEdge(graph_reader, the_map.nodes, "", "I", baldr::GraphId{}, 102);
  EXPECT_NE(HI_edge, nullptr);
  EXPECT_NE(IH_edge, nullptr);

  GraphId node_id = HI_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(HI_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(HI_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 4);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "19");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "Queen Street");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(3).text(), "rue Queen");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_fr_QuebecCanada, CheckGuideSigns) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"promenade Vanier/Vanier Parkway/19",
                                           "promenade Vanier/Vanier Parkway/19",
                                           "promenade Vanier/Vanier Parkway/19", "",
                                           "Albert Street/rue Albert"});

  // Verify starting on Vanier Parkway/promenade Vanier/19
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "promenade Vanier");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Vanier Parkway");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "19");

  ++maneuver_index;
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets_size(),
            2);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(0)
                .text(),
            "rue Albert");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_onto_streets(1)
                .text(),
            "Albert Street");

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations_size(),
            1);

  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .guide_toward_locations(0)
                .text(),
            "Centretown");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  GraphId node_id = PD_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(PD_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 2);

  // note flipped on purpose
  ASSERT_EQ(edge_signs.at(0).text(), "rue Albert");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "fr");

  ASSERT_EQ(edge_signs.at(1).text(), "Albert Street");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(2).text(), "Centretown");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_fr_QuebecCanada, CheckNonJunctionName) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "Q"}, "auto");
  gurka::assert::raw::expect_path(result,
                                  {"promenade Vanier/Vanier Parkway/19",
                                   "promenade Vanier/Vanier Parkway/19",
                                   "promenade Vanier/Vanier Parkway/19",
                                   "promenade Vanier/Vanier Parkway/19", "Albert Street/rue Albert"});

  // Verify starting on Vanier Parkway/promenade Vanier/19
  int maneuver_index = 0;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 3);
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "promenade Vanier");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "Vanier Parkway");
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(2).value(),
            "19");

  // Verify street name language tag
  ++maneuver_index;
  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name_size(), 2);

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(0).value(),
            "Albert Street");

  EXPECT_EQ(result.directions().routes(0).legs(0).maneuver(maneuver_index).street_name(1).value(),
            "rue Albert");

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Albert Street");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "rue Albert");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "fr");

  // No junction should exist here.  Named junctions are not allowed in CA
  EXPECT_EQ(result.directions()
                .routes(0)
                .legs(0)
                .maneuver(maneuver_index)
                .sign()
                .junction_names_size(),
            0);
}

class RouteWithStreetnameAndSign_en_ms_ta_zh_Singapore : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";
    const gurka::ways ways = {
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Rochor"},
          {"name:en", "Rochor"},
          {"name:ms", "Rochor"},
          {"name:ta", "ரோச்சோர்"},
          {"name:zh", "梧槽"},
          {"name:en:pronunciation:nt-sampa", "English Language pronunciation"},
          {"name:zh:pronunciation:nt-sampa", "Native zh Language pronunciation"},
          {"name:pronunciation:nt-sampa", "Native Language pronunciation"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {103.87149, 1.32510});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_ms_ta_zh_Singapore, CheckForwardNames) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_en_ms_ta_zh_Singapore";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Rochor/梧槽/Rochor/ரோச்சோர்"});
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 4);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
  ASSERT_EQ(names_and_types.at(0).first, "Rochor");

  lang_iter = languages.find(1);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "zh");
  ASSERT_EQ(names_and_types.at(1).first, "梧槽");

  lang_iter = languages.find(2);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ms");
  ASSERT_EQ(names_and_types.at(2).first, "Rochor");

  lang_iter = languages.find(3);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ta");
  ASSERT_EQ(names_and_types.at(3).first, "ரோச்சோர்");

  std::unordered_map<uint8_t, std::pair<uint8_t, std::string>> pronunciations =
      edgeinfo.GetPronunciationsMap();
  ASSERT_EQ(pronunciations.size(), 0);
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_ms_ta_zh_Singapore, CheckBackwardNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Rochor/梧槽/Rochor/ரோச்சோர்"});
  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 4);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
  ASSERT_EQ(names_and_types.at(0).first, "Rochor");

  lang_iter = languages.find(1);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "zh");
  ASSERT_EQ(names_and_types.at(1).first, "梧槽");

  lang_iter = languages.find(2);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ms");
  ASSERT_EQ(names_and_types.at(2).first, "Rochor");

  lang_iter = languages.find(3);
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ta");
  ASSERT_EQ(names_and_types.at(3).first, "ரோச்சோர்");

  std::unordered_map<uint8_t, std::pair<uint8_t, std::string>> pronunciations =
      edgeinfo.GetPronunciationsMap();
  ASSERT_EQ(pronunciations.size(), 0);
}

class RouteWithStreetnameAndSign_ja_en_JapanPronunciations : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
                       J
                       |
                       |
                       |
                       I
                      /|\
                    /  |  \
                  /    |    \
           L----K-------------H----G
           A----B-------------E----F
                  \    |    /
                    \  |  /
                      \|/
                       C
                       |
                       |
                       |
                       D
               O------PM------Q
                       |
                       |
                       |
                       N

    )";

    const gurka::ways ways = {
        {"ABEF",
         {{"highway", "motorway"},
          {"osm_id", "98"},
          {"name", "首都高速6号向島線"},
          {"name:pronunciation", "ja_name_pronunciation"},
          // TODO:  When multiple languages are supported, make sure pronunciations reference the
          // correct name in the list.  For now these are tossed.
          //{"name:en", "Shuto Expressway Route 6 Mukojima Line"},
          {"name:es", "Ruta 6 Mukojima de la Autopista Shuto"},
          {"name:ja", "首都高速6号向島線"},
          {"name:ru", "Шоссе Мукодзима"},
          {"ref", "6"},
          {"oneway", "yes"}}},
        {"GHKL",
         {{"highway", "motorway"},
          {"osm_id", "99"},
          {"name", "首都高速6号向島線"},
          {"name:pronunciation", "ja_name_pronunciation"},
          //{"name:en", "Shuto Expressway Route 6 Mukojima Line"},
          {"name:es", "Ruta 6 Mukojima de la Autopista Shuto"},
          {"name:ja", "首都高速6号向島線"},
          {"name:ru", "Шоссе Мукодзима"},
          {"ref", "6"},
          {"oneway", "yes"}}},
        {"JICDMN",
         {{"highway", "primary"},
          {"osm_id", "100"},
          {"name", "常磐道;東北道"},
          {"name:en", "Joban Expressway;Tohoku Expressway"},
          {"ref", "E6;E4"}}},
        {"BC",
         {{"highway", "motorway_link"},
          {"osm_id", "103"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination", "常磐道;東北道"},
          {"destination:lang:ja", "常磐道;東北道"},
          // TODO:  For now, pronunciations win over languages.  We need to update the language in
          // graphbuilder for pronunciations.
          {"destination:pronunciation",
           "ja_destination_pronunciation_1;ja_destination_pronunciation_2"},
          // TODO:  When multiple languages are supported, make sure pronunciations reference the
          // correct sign in the list.  For now these are tossed.
          //{"destination:lang:en", "Joban Expressway;Tohoku Expressway"},
          {"destination:street", "清洲橋通り"},
          {"destination:street:lang:en", "Kiyosubashi-dori Avenue"},
          {"destination:street:lang:es", "Calle Kiyosubashi"},
          {"destination:street:lang:ja", "清洲橋通り"},
          {"destination:street:lang:ja_rm", "Kiyosubashi Dōri"},
          {"destination:ref", "E6;E4"}}},
        {"CE",
         {{"highway", "motorway_link"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination:ref", "E6;E4"}}},
        {"HI",
         {{"highway", "motorway_link"},
          {"osm_id", "102"},
          {"name", ""},
          {"oneway", "yes"},
          {"junction:ref", "26B"},
          {"destination:street:to", "清澄通り"},
          {"destination:street:to:lang:en", "Kiyosumi-dori"},
          {"destination:street:to:lang:es", "Calle Kiyosumi"},
          {"destination:street:to:lang:ja_kana", "きよすみどおり"},
          {"destination:ref:to", "M4"}}},
        {"IK",
         {{"highway", "motorway_link"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination:ref", "E6;E4"}}},
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "国技館通り"},
          {"name:en", "Kokugikan-dori"},
          {"name:es", "Calle Kokugikan"},
          {"name:ja_kana", "こくぎかんどおり"},
          {"name:pronunciation", "ja_name_pronunciation_1;ja_name_pronunciation_2"}}},
        {"DP",
         {{"highway", "secondary_link"},
          {"osm_id", "104"},
          {"name", ""},
          {"oneway", "yes"},
          {"destination", "大和街道"},
          {"destination:street", "国技館通り"},
          {"destination:street:lang:en", "Kokugikan-dori"},
          {"destination:street:lang:es", "Calle Kokugikan"},
          {"destination:street:lang:ja_kana", "こくぎかんどおり"}}},
    };

    const gurka::nodes nodes = {
        {"M",
         {{"highway", "traffic_signals"}, {"name", "両国二丁目"}, {"name:en", "Ryogoku 2-chome"}}}};

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {139.79079, 35.69194});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, nodes, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ja_en_JapanPronunciations, CheckStreetNamesAndSigns1) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_ja_en_JapanPronunciations";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"A", "D"}, "auto");
  gurka::assert::raw::expect_path(result, {"6/首都高速6号向島線", "",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId ABEF_edge_id;
  const DirectedEdge* ABEF_edge = nullptr;
  GraphId FEBA_edge_id;
  const DirectedEdge* FEBA_edge = nullptr;
  std::tie(ABEF_edge_id, ABEF_edge, FEBA_edge_id, FEBA_edge) =
      findEdge(graph_reader, the_map.nodes, "", "F", baldr::GraphId{}, 98);
  EXPECT_NE(ABEF_edge, nullptr);
  EXPECT_NE(FEBA_edge, nullptr);

  GraphId BC_edge_id;
  const DirectedEdge* BC_edge = nullptr;
  GraphId CB_edge_id;
  const DirectedEdge* CB_edge = nullptr;
  std::tie(BC_edge_id, BC_edge, CB_edge_id, CB_edge) =
      findEdge(graph_reader, the_map.nodes, "", "C", baldr::GraphId{}, 103);
  EXPECT_NE(BC_edge, nullptr);
  EXPECT_NE(CB_edge, nullptr);

  GraphId JICDMN_edge_id;
  const DirectedEdge* JICDMN_edge = nullptr;
  GraphId NMDCIJ_edge_id;
  const DirectedEdge* NMDCIJ_edge = nullptr;
  std::tie(JICDMN_edge_id, JICDMN_edge, NMDCIJ_edge_id, NMDCIJ_edge) =
      findEdge(graph_reader, the_map.nodes, "", "N", baldr::GraphId{}, 100);
  EXPECT_NE(JICDMN_edge, nullptr);
  EXPECT_NE(NMDCIJ_edge, nullptr);

  GraphId node_id = ABEF_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(ABEF_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, std::pair<uint8_t, std::string>> pronunciations =
      edgeinfo.GetPronunciationsMap();

  ASSERT_EQ(names_and_types.size(), 2);
  ASSERT_EQ(languages.size(), 1);
  ASSERT_EQ(pronunciations.size(), 1);

  ASSERT_EQ(names_and_types.at(0).first, "6");
  std::unordered_map<uint8_t, std::pair<uint8_t, std::string>>::const_iterator pronunciations_iter =
      pronunciations.find(0);
  ASSERT_EQ(pronunciations_iter, pronunciations.end());

  ASSERT_EQ(names_and_types.at(1).first, "首都高速6号向島線");
  pronunciations_iter = pronunciations.find(1);
  ASSERT_NE(pronunciations_iter, pronunciations.end());
  ASSERT_EQ((pronunciations_iter->second).second, "ja_name_pronunciation");

  node_id = BC_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(BC_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(BC_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 7);
  ASSERT_EQ(linguistics.size(), 4);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "E6");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "E4");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(3).text(), "清洲橋通り");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(4).text(), "Kiyosubashi-dori Avenue");
  iter = linguistics.find(4);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(5).text(), "常磐道");
  iter = linguistics.find(5);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "none");
  ASSERT_EQ(std::get<kLinguisticMapTuplePronunciationIndex>(iter->second),
            "ja_destination_pronunciation_1");

  ASSERT_EQ(edge_signs.at(6).text(), "東北道");
  iter = linguistics.find(6);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "none");
  ASSERT_EQ(std::get<kLinguisticMapTuplePronunciationIndex>(iter->second),
            "ja_destination_pronunciation_2");

  node_id = JICDMN_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);
  edgeinfo = tile->edgeinfo(JICDMN_edge);
  types.clear();
  names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  languages = edgeinfo.GetLanguageMap();

  ASSERT_EQ(names_and_types.size(), 6);

  ASSERT_EQ(names_and_types.at(0).first, "常磐道");
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ja");

  ASSERT_EQ(names_and_types.at(1).first, "東北道");
  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ja");

  ASSERT_EQ(names_and_types.at(2).first, "Joban Expressway");
  lang_iter = languages.find(2);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  ASSERT_EQ(names_and_types.at(3).first, "Tohoku Expressway");
  lang_iter = languages.find(3);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  ASSERT_EQ(names_and_types.at(4).first, "E6");
  lang_iter = languages.find(4);
  ASSERT_EQ(lang_iter, languages.end());

  ASSERT_EQ(names_and_types.at(5).first, "E4");
  lang_iter = languages.find(5);
  ASSERT_EQ(lang_iter, languages.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ja_en_JapanPronunciations, CheckStreetNamesAndSigns2) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"G", "J"}, "auto");
  gurka::assert::raw::expect_path(result, {"6/首都高速6号向島線", "",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId HI_edge_id;
  const DirectedEdge* HI_edge = nullptr;
  GraphId IH_edge_id;
  const DirectedEdge* IH_edge = nullptr;
  std::tie(HI_edge_id, HI_edge, IH_edge_id, IH_edge) =
      findEdge(graph_reader, the_map.nodes, "", "I", baldr::GraphId{}, 102);
  EXPECT_NE(HI_edge, nullptr);
  EXPECT_NE(IH_edge, nullptr);

  GraphId node_id = HI_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(HI_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(HI_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 4);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "26B");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(1).text(), "M4");
  iter = linguistics.find(1);
  ASSERT_EQ(iter, linguistics.end());

  ASSERT_EQ(edge_signs.at(2).text(), "清澄通り");
  iter = linguistics.find(2);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(3).text(), "Kiyosumi-dori");
  iter = linguistics.find(3);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ja_en_JapanPronunciations, CheckGuideSigns) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "", "国技館通り/Kokugikan-dori"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId DP_edge_id;
  const DirectedEdge* DP_edge = nullptr;
  GraphId PD_edge_id;
  const DirectedEdge* PD_edge = nullptr;
  std::tie(DP_edge_id, DP_edge, PD_edge_id, PD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "P", baldr::GraphId{}, 104);
  EXPECT_NE(DP_edge, nullptr);
  EXPECT_NE(PD_edge, nullptr);

  GraphId node_id = PD_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(PD_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 0);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(DP_edge_id.id(), linguistics);

  ASSERT_EQ(edge_signs.size(), 3);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "国技館通り");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(1).text(), "Kokugikan-dori");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");

  ASSERT_EQ(edge_signs.at(2).text(), "大和街道");
  iter = linguistics.find(2);
  ASSERT_EQ(iter, linguistics.end());
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_ja_en_JapanPronunciations, CheckNonJunctionName) {
  auto result = gurka::do_action(valhalla::Options::route, the_map, {"J", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "常磐道/東北道/Joban Expressway/Tohoku Expressway/E6/E4",
                                           "国技館通り/Kokugikan-dori"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "国技館通り");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "ja");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Kokugikan-dori");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  GraphId DM_edge_id;
  const DirectedEdge* DM_edge = nullptr;
  GraphId MD_edge_id;
  const DirectedEdge* MD_edge = nullptr;
  std::tie(DM_edge_id, DM_edge, MD_edge_id, MD_edge) =
      findEdge(graph_reader, the_map.nodes, "", "M", baldr::GraphId{}, 100);
  EXPECT_NE(DM_edge, nullptr);
  EXPECT_NE(MD_edge, nullptr);

  node_id = DM_edge->endnode();
  tile = graph_reader.GetGraphTile(node_id);

  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>> linguistics;
  std::vector<SignInfo> edge_signs = tile->GetSigns(node_id.id(), linguistics, true);

  ASSERT_EQ(edge_signs.size(), 2);
  ASSERT_EQ(linguistics.size(), 2);

  ASSERT_EQ(edge_signs.at(0).text(), "両国二丁目");
  std::unordered_map<uint8_t, std::tuple<uint8_t, uint8_t, std::string>>::const_iterator iter =
      linguistics.find(0);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "ja");

  ASSERT_EQ(edge_signs.at(1).text(), "Ryogoku 2-chome");
  iter = linguistics.find(1);
  ASSERT_NE(iter, linguistics.end());
  ASSERT_EQ(to_string(
                static_cast<Language>(std::get<kLinguisticMapTupleLanguageIndex>(iter->second))),
            "en");
}

class RouteWithStreetnameAndSign_en_USMultiWithNameDash : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Pamplona South - Airport"},
          {"name:en", "Pamplona South - Airport"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {-76.69980, 40.25882});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_USMultiWithNameDash, CheckForwardNames) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_en_USMultiWithNameDash";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Pamplona South/Airport"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Pamplona South");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Airport");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_USMultiWithNameDash, CheckBackwardNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Pamplona South/Airport"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Pamplona South");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Airport");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}

class RouteWithStreetnameAndSign_en_USMultiWithNameSlash : public ::testing::Test {
protected:
  valhalla::gurka::map BuildPBF(const std::string& workdir) {
    constexpr double gridsize_metres = 100;

    const std::string ascii_map = R"(
               O------PM------Q
    )";

    const gurka::ways ways = {
        {"OPMQ",
         {{"highway", "secondary"},
          {"osm_id", "103"},
          {"name", "Pamplona South / Airport"},
          {"name:en", "Pamplona South / Airport"}}},
    };

    if (!filesystem::exists(workdir)) {
      bool created = filesystem::create_directories(workdir);
      EXPECT_TRUE(created);
    }

    constexpr double gridsize = 100;

    const auto layout =
        gurka::detail::map_to_coordinates(ascii_map, gridsize_metres, {-76.69980, 40.25882});

    auto pbf_filename = workdir + "/map.pbf";
    detail::build_pbf(layout, ways, {}, {}, pbf_filename);

    valhalla::gurka::map result;
    result.nodes = layout;
    return result;
  }
};

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_USMultiWithNameSlash, CheckForwardNames) {

  const std::string workdir =
      "test/data/gurka_language_with_streetname_and_sign_en_USMultiWithNameSlash";

  if (!filesystem::exists(workdir)) {
    bool created = filesystem::create_directories(workdir);
    EXPECT_TRUE(created);
  }

  the_map = BuildPBF(workdir);

  const std::string sqlite = {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"};
  the_map.config =
      test::make_config(workdir,
                        {{"mjolnir.admin", {VALHALLA_SOURCE_DIR "test/data/language_admin.sqlite"}},
                         {"mjolnir.tile_dir", workdir + "/tiles"}});

  std::vector<std::string> input_files = {workdir + "/map.pbf"};

  build_tile_set(the_map.config, input_files, mjolnir::BuildStage::kInitialize,
                 mjolnir::BuildStage::kValidate, false);

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"O", "Q"}, "auto");
  gurka::assert::raw::expect_path(result, {"Pamplona South/Airport"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = OPMQ_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(OPMQ_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Pamplona South");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Airport");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}

///////////////////////////////////////////////////////////////////////////////
TEST_F(RouteWithStreetnameAndSign_en_USMultiWithNameSlash, CheckBackwardNames) {

  auto result = gurka::do_action(valhalla::Options::route, the_map, {"Q", "O"}, "auto");
  gurka::assert::raw::expect_path(result, {"Pamplona South/Airport"});

  GraphReader graph_reader(the_map.config.get_child("mjolnir"));

  GraphId OPMQ_edge_id;
  const DirectedEdge* OPMQ_edge = nullptr;
  GraphId QMPO_edge_id;
  const DirectedEdge* QMPO_edge = nullptr;
  std::tie(OPMQ_edge_id, OPMQ_edge, QMPO_edge_id, QMPO_edge) =
      findEdge(graph_reader, the_map.nodes, "", "Q", baldr::GraphId{}, 103);
  EXPECT_NE(OPMQ_edge, nullptr);
  EXPECT_NE(QMPO_edge, nullptr);

  GraphId node_id = QMPO_edge->endnode();
  auto tile = graph_reader.GetGraphTile(node_id);
  auto edgeinfo = tile->edgeinfo(QMPO_edge);
  std::vector<uint8_t> types;
  auto names_and_types = edgeinfo.GetNamesAndTypes(types, true);
  ASSERT_EQ(names_and_types.size(), 2);

  std::unordered_map<uint8_t, uint8_t> languages = edgeinfo.GetLanguageMap();
  std::unordered_map<uint8_t, uint8_t>::const_iterator lang_iter = languages.find(0);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(0).first, "Pamplona South");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");

  lang_iter = languages.find(1);
  ASSERT_NE(lang_iter, languages.end());
  ASSERT_EQ(names_and_types.at(1).first, "Airport");
  ASSERT_EQ(to_string(static_cast<Language>(lang_iter->second)), "en");
}