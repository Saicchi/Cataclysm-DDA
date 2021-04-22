#include "catch/catch.hpp"
#include "map_helpers.h"
#include "player_helpers.h"
#include "activity_scheduling_helper.h"

#include "map.h"
#include "character.h"
#include "avatar.h"
#include "calendar.h"
#include "flag.h"
#include "point.h"
#include "activity_actor_definitions.h"


static const activity_id ACT_CRACKING( "ACT_CRACKING" );

static const skill_id skill_traps( "traps" );

static const json_character_flag json_flag_SUPER_HEARING( "SUPER_HEARING" );

static const proficiency_id proficiency_prof_safecracking( "prof_safecracking" );

TEST_CASE( "safecracking", "[activity][safecracking][devices]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

    SECTION( "safecracking testing time" ) {

        auto safecracking_setup = [&dummy]( int perception,
        int skill_level, bool has_proficiency ) -> void {
            dummy.per_max = perception;
            dummy.set_skill_level( skill_traps, skill_level );

            REQUIRE( dummy.get_per() == perception );
            REQUIRE( dummy.get_skill_level( skill_traps ) == skill_level );
            if( has_proficiency )
            {
                dummy.add_proficiency( proficiency_prof_safecracking );
                REQUIRE( dummy.has_proficiency( proficiency_prof_safecracking ) );
            } else
            {
                dummy.lose_proficiency( proficiency_prof_safecracking );
                REQUIRE( !dummy.has_proficiency( proficiency_prof_safecracking ) );
            }
        };

        GIVEN( "player with default perception, 0 devices skill and no safecracking proficiency" ) {
            safecracking_setup( 8, 0, false );
            THEN( "cracking time is 10 hours and 30 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 630_minutes );
            }
        }

        GIVEN( "player with 10 perception, 4 devices skill and no safecracking proficiency" ) {
            safecracking_setup( 10, 4, false );
            THEN( "cracking time is 5 hours and 30 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 330_minutes );
            }
        }

        GIVEN( "player with 10 perception, 4 devices skill and safecracking proficiency" ) {
            safecracking_setup( 10, 4, true );
            THEN( "cracking time is 1 hour and 50 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 110_minutes );
            }
        }

        GIVEN( "player with 8 perception, 10 devices skill and safecracking proficiency" ) {
            safecracking_setup( 8, 10, true );
            THEN( "cracking time is 30 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 30_minutes );
            }
        }

        GIVEN( "player with 8 perception, 10 devices skill and no safecracking proficiency" ) {
            safecracking_setup( 8, 10, false );
            THEN( "cracking time is 90 minutes" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 90_minutes );
            }
        }

        GIVEN( "player with 1 perception, 0 devices skill and no safecracking proficiency" ) {
            safecracking_setup( 1, 0, false );
            THEN( "cracking time is 14 hours" ) {
                const time_duration time = safecracking_activity_actor::safecracking_time( dummy );
                CHECK( time == 14_hours );
            }
        }
    }

    SECTION( "safecracking tools test" ) {
        clear_avatar();
        clear_map();

        tripoint safe;
        dummy.setpos( safe + tripoint_east );

        map &mp = get_map();
        dummy.activity = player_activity( safecracking_activity_actor( safe ) );
        dummy.activity.start_or_resume( dummy, false );

        GIVEN( "player without the required tools" ) {
            mp.furn_set( safe, f_safe_l );
            REQUIRE( !dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SUPER_HEARING ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            WHEN( "player tries safecracking" ) {
                process_activity( dummy );
                THEN( "activity is canceled" ) {
                    CHECK( mp.furn( safe ) == f_safe_l );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.i_add( item( "stethoscope" ) );
            mp.furn_set( safe, f_safe_l );
            REQUIRE( dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SUPER_HEARING ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            WHEN( "player completes the safecracking activity" ) {
                process_activity( dummy );
                THEN( "safe is unlocked" ) {
                    CHECK( mp.furn( safe ) == f_safe_c );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.worn.clear();
            dummy.remove_weapon();
            dummy.add_bionic( bionic_id( "bio_ears" ) );
            mp.furn_set( safe, f_safe_l );
            REQUIRE( !dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( dummy.has_flag( json_flag_SUPER_HEARING ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            WHEN( "player completes the safecracking activity" ) {
                process_activity( dummy );
                THEN( "safe is unlocked" ) {
                    CHECK( mp.furn( safe ) == f_safe_c );
                }
            }
        }

        GIVEN( "player has a stethoscope" ) {
            dummy.clear_bionics();
            dummy.i_add( item( "stethoscope" ) );
            mp.furn_set( safe, f_safe_l );
            REQUIRE( dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( !dummy.has_flag( json_flag_SUPER_HEARING ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            WHEN( "player is safecracking" ) {
                dummy.moves += dummy.get_speed();
                for( int i = 0; i < 100; ++i ) {
                    dummy.activity.do_turn( dummy );
                }

                THEN( "player loses their stethoscope" ) {
                    dummy.worn.clear();
                    dummy.remove_weapon();
                    REQUIRE( !dummy.has_item_with_flag( flag_SAFECRACK ) );

                    process_activity( dummy );
                    THEN( "activity is canceled" ) {
                        CHECK( mp.furn( safe ) == f_safe_l );
                    }
                }
            }
        }
    }

    SECTION( "safecracking proficiency test" ) {

        auto get_safecracking_time = [&dummy]() -> time_duration {
            const std::vector<display_proficiency> profs = dummy.display_proficiencies();
            const auto iter = std::find_if( profs.begin(), profs.end(),
            []( const display_proficiency & p ) -> bool {
                return p.id == proficiency_prof_safecracking;
            } );
            if( iter == profs.end() )
            {
                return time_duration();
            }
            return iter->spent;
        };

        clear_avatar();
        clear_map();

        tripoint safe;
        dummy.setpos( safe + tripoint_east );

        map &mp = get_map();
        dummy.activity = player_activity( safecracking_activity_actor( safe ) );
        dummy.activity.start_or_resume( dummy, false );

        GIVEN( "player cracks one safe" ) {
            dummy.i_add( item( "stethoscope" ) );
            mp.furn_set( safe, f_safe_l );
            REQUIRE( dummy.has_item_with_flag( flag_SAFECRACK ) );
            REQUIRE( dummy.activity.id() == ACT_CRACKING );
            REQUIRE( mp.furn( safe ) == f_safe_l );

            REQUIRE( !dummy.has_proficiency( proficiency_prof_safecracking ) );

            dummy.set_focus( 100 );
            REQUIRE( dummy.get_focus() == 100 );

            const time_duration time_before = get_safecracking_time();

            process_activity( dummy );
            REQUIRE( mp.furn( safe ) == f_safe_c );
            THEN( "proficiency given is less than 90 minutes" ) {
                const time_duration time_after = get_safecracking_time();
                REQUIRE( time_after > 0_seconds );

                const time_duration time_delta = time_after - time_before;
                CHECK( time_delta > 5_minutes );
                CHECK( time_delta < 90_minutes );
            }
        }
    }
}
