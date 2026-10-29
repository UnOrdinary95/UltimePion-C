cmake_minimum_required(VERSION 3.16)
project(UltimePion LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)

add_executable(ultime_pion src/ultime_pion.c)
include(CTest)
if(BUILD_TESTING)
    add_executable(test_game tests/test_game.c)
    foreach(case IN ITEMS initialization validity boundaries endings
            independent_moves double_invalid invalid_player1 invalid_player2
            pawn_duel knight_duel knight_beats_pawn pawn_loses_to_knight
            pawn_swap knight_swap escape stationary_capture invalid_piece_captured
            promotion_player1 promotion_player2 double_promotion
            redeploy_player1 redeploy_player2 double_redeploy redeploy_collision
            redeploy_occupied redeploy_after_knight_capture lost_pawn_no_promotion
            last_pawn_promoted last_pawns_duel second_penalties
            platon_forbidden_redeploy forbidden_redeploy_player2
            missing_redeploy_player1 missing_redeploy_player2
            out_of_bounds_redeploy_player1 out_of_bounds_redeploy_player2)
        add_test(NAME game.${case} COMMAND test_game ${case})
        set_tests_properties(game.${case} PROPERTIES TIMEOUT 10)
    endforeach()
    foreach(case IN ITEMS roundtrip compact_input turn)
        add_test(NAME io.${case} COMMAND ${CMAKE_COMMAND}
            -DPROGRAM=$<TARGET_FILE:test_game>
            -DMODE=${case}
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/${case}.in
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/${case}.out
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_output.cmake)
    endforeach()
    foreach(case IN ITEMS draw player1 player2)
        add_test(NAME cli.${case} COMMAND ${CMAKE_COMMAND}
            -DPROGRAM=$<TARGET_FILE:ultime_pion>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cli_${case}.in
            "-DEXPECTED_MESSAGE=${case}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_output.cmake)
    endforeach()
endif()

foreach(target IN ITEMS ultime_pion test_game)
    if(TARGET ${target} AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(ENABLE_SANITIZERS)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endforeach()
