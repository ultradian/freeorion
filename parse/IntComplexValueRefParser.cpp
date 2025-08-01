#include "ValueRefParser.h"
#include "EnumValueRefRules.h"

#include "MovableEnvelope.h"
#include "../universe/ValueRefs.h"

// for definition of PlanetType
#include "../universe/Planet.h"
#include "../universe/System.h"

#include <boost/phoenix.hpp>
#include <boost/spirit/include/qi_as.hpp>


namespace parse {
    namespace detail {
        struct cast_T_to_int {
            template <typename T>
            int operator() (T t) const
            { return static_cast<int>(t); }
        };
    }

    std::unique_ptr<ValueRef::ValueRef<int>> planet_type_as_int(
        const parse::detail::MovableEnvelope<ValueRef::ValueRef<PlanetType>>& ref_envelope,
        bool& pass)
    {
        auto vref = ref_envelope.OpenEnvelope(pass);
        return std::make_unique<ValueRef::StaticCast<::PlanetType, int>>(std::move(vref));
    }
    BOOST_PHOENIX_ADAPT_FUNCTION(std::unique_ptr<ValueRef::ValueRef<int>>,
                                 planet_type_as_int_, planet_type_as_int, 2)

    int_complex_parser_grammar::int_complex_parser_grammar(
        const parse::lexer& tok,
        detail::Labeller& label,
        const int_arithmetic_rules& _int_arith_rules,
        const parse::detail::condition_parser_grammar& condition_parser,
        const detail::value_ref_grammar<std::string>& string_grammar
    ) :
        int_complex_parser_grammar::base_type(start, "int_complex_parser_grammar"),
        int_rules(_int_arith_rules),
        ship_part_class_enum(tok),
        planet_type_rules(tok, label, condition_parser)
    {
        namespace phoenix = boost::phoenix;
        namespace qi = boost::spirit::qi;

        using phoenix::new_;

        qi::_1_type _1;
        qi::_2_type _2;
        qi::_3_type _3;
        qi::_4_type _4;
        qi::_val_type _val;
        qi::eps_type eps;
        qi::_pass_type _pass;
        const boost::phoenix::function<detail::construct_movable> construct_movable_;
        const boost::phoenix::function<detail::deconstruct_movable> deconstruct_movable_;
        const boost::phoenix::function<detail::cast_T_to_int> int_cast_;

        game_rule
            = (   tok.GameRule_
                > label(tok.name_) > string_grammar
              ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, nullptr, nullptr, nullptr, deconstruct_movable_(_2, _pass), nullptr)) ]
            ;

         empire_name_ref
            =   (
                    (   tok.BuildingTypesOwned_
                    |   tok.BuildingTypesProduced_
                    |   tok.BuildingTypesScrapped_
                    |   tok.SpeciesColoniesOwned_
                    |   tok.SpeciesPlanetsBombed_
                    |   tok.SpeciesPlanetsDepoped_
                    |   tok.SpeciesPlanetsInvaded_
                    |   tok.SpeciesShipsDestroyed_
                    |   tok.SpeciesShipsLost_
                    |   tok.SpeciesShipsOwned_
                    |   tok.SpeciesShipsProduced_
                    |   tok.SpeciesShipsScrapped_
                    |   tok.TurnTechResearched_
                    |   tok.TurnPolicyAdopted_
                    |   tok.TurnsSincePolicyAdopted_
                    |   tok.CumulativeTurnsPolicyAdopted_
                    |   tok.LatestTurnPolicyAdopted_
                    |   tok.NumPoliciesAdopted_
                    )
                >  -(   label(tok.empire_) > int_rules.expr)
                >  -(   label(tok.name_) >   string_grammar)
                ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(
                        _1, deconstruct_movable_(_2, _pass), nullptr, nullptr, deconstruct_movable_(_3, _pass), nullptr))
                  ]
            ;

         empire_id_ref
            = (
                    tok.TurnSystemExplored_
                >-( label(tok.empire_) > int_rules.expr )
                >-( label(tok.id_)     > int_rules.expr )
              ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(
                        _1, deconstruct_movable_(_2, _pass), deconstruct_movable_(_3, _pass), nullptr, nullptr, nullptr))
                ]
            ;

         empire_ships_destroyed
            =   (
                    tok.EmpireShipsDestroyed_
                >-( label(tok.empire_) > int_rules.expr )
                >-( label(tok.empire_) > int_rules.expr )
                ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, deconstruct_movable_(_2, _pass), deconstruct_movable_(_3, _pass), nullptr, nullptr, nullptr)) ]
            ;

        jumps_between
            = (  tok.JumpsBetween_
               > label(tok.object_)
               > ( int_rules.expr
                   // "cast" the ValueRef::Statistic<int> into
                   // ValueRef::ValueRef<int> so the alternative contains a
                   // single type
                   | qi::as<parse::detail::MovableEnvelope<ValueRef::ValueRef<int>>>()[int_rules.statistic_expr])
               > label(tok.object_)
               > (int_rules.expr
                  | qi::as<parse::detail::MovableEnvelope<ValueRef::ValueRef<int>>>()[int_rules.statistic_expr])
              ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, deconstruct_movable_(_2, _pass), deconstruct_movable_(_3, _pass), nullptr, nullptr, nullptr)) ]
            ;

        //jumps_between_by_empire_supply
        //    =   (
        //                tok.JumpsBetweenByEmpireSupplyConnections_ [ _a = construct<std::string>(_1) ]
        //            >   label(tok.object_) >>   int_rules.expr [ _b = _1 ]
        //            >   label(tok.object_) >>   int_rules.expr [ _c = _1 ]
        //            >   label(tok.empire_) >>   int_rules.expr [ _f = _1 ]
        //        ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_a, deconstruct_movable_(_b, _pass), deconstruct_movable_(_c, _pass), deconstruct_movable_(_f, _pass), deconstruct_movable_(_d, _pass), deconstruct_movable_(_e, _pass))) ]
        //    ;

        outposts_owned
            =   (
                    tok.OutpostsOwned_
                >-( label(tok.empire_) > int_rules.expr )
                ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, deconstruct_movable_(_2, _pass), nullptr, nullptr, nullptr, nullptr)) ]
            ;

        parts_in_ship_design
            =   (
                    tok.PartsInShipDesign_
                >-( label(tok.name_)   > string_grammar )
                > ( label(tok.design_) > int_rules.expr )
            ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, deconstruct_movable_(_3, _pass), nullptr, nullptr, deconstruct_movable_(_2, _pass), nullptr)) ]
            ;

        part_class_in_ship_design
            =   (
                tok.PartOfClassInShipDesign_
                //> ( label(tok.class_) >>
                //    as_string [ ship_part_class_enum ]
                //    [ _d = construct_movable_(new_<ValueRef::Constant<std::string>>(_1)) ]
                //  )
                > ( label(tok.class_) >
                    (   tok.ShortRange_       | tok.FighterBay_   | tok.FighterWeapon_
                      | tok.Shield_           | tok.Armour_
                      | tok.Troops_           | tok.Detection_    | tok.Stealth_
                      | tok.Fuel_             | tok.Colony_       | tok.Speed_
                      | tok.General_          | tok.Bombard_      | tok.Research_
                      | tok.Industry_         | tok.Influence_    | tok.ProductionLocation_
                    )
                  )
                > ( label(tok.design_) > int_rules.expr)
            ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(
                _1, deconstruct_movable_(_3, _pass), nullptr, nullptr,
                deconstruct_movable_(construct_movable_(new_<ValueRef::Constant<std::string>>(_2)), _pass),
                nullptr)) ]
            ;

        part_class_as_int
            = ( label(tok.class_) > ship_part_class_enum )
              [ _val = construct_movable_(new_<ValueRef::Constant<int>>(int_cast_(_1))) ]
        ;

        ship_parts_owned
            =   (
                     tok.ShipPartsOwned_
                > -( label(tok.empire_) > int_rules.expr )
                > -( label(tok.name_)   > string_grammar )
                > -part_class_as_int
                ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(
                    _1,
                    deconstruct_movable_(_2, _pass), deconstruct_movable_(_4, _pass), nullptr,
                    deconstruct_movable_(_3, _pass), nullptr)) ]
            ;

        empire_design_ref
            =   (
                    (   tok.ShipDesignsDestroyed_
                    |   tok.ShipDesignsLost_
                    |   tok.ShipDesignsInProduction_
                    |   tok.ShipDesignsOwned_
                    |   tok.ShipDesignsProduced_
                    |   tok.ShipDesignsScrapped_
                    )
                >  -(   label(tok.empire_) > int_rules.expr )
                >  -(   label(tok.design_) > string_grammar )
                ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, deconstruct_movable_(_2, _pass), nullptr, nullptr, deconstruct_movable_(_3, _pass), nullptr)) ]
            ;

        slots_in_hull
            =   (
                    tok.SlotsInHull_
                >   label(tok.name_) > string_grammar
            ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, nullptr, nullptr, nullptr, deconstruct_movable_(_2, _pass), nullptr)) ]
            ;

        slots_in_ship_design
            =   (
                    tok.SlotsInShipDesign_
                >   label(tok.design_) > int_rules.expr
                ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, deconstruct_movable_(_2, _pass), nullptr, nullptr, nullptr, nullptr)) ]
            ;

        special_added_on_turn
            =   (
                    tok.SpecialAddedOnTurn_
                >-( label(tok.name_)   > string_grammar )
                >-( label(tok.object_) > int_rules.expr )
            ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(_1, deconstruct_movable_(_3, _pass), nullptr, nullptr, deconstruct_movable_(_2, _pass), nullptr)) ]
            ;

        planet_type_difference
            =   (
                    tok.PlanetTypeDifference_
                    > label(tok.from_) > planet_type_rules.expr
                    > label(tok.to_)   > planet_type_rules.expr
                ) [ _val = construct_movable_(new_<ValueRef::ComplexVariable<int>>(
                    _1, planet_type_as_int_(_2, _pass), planet_type_as_int_(_3, _pass),
                    nullptr, nullptr, nullptr)) ]
            ;

        start
            %=  game_rule
            |   empire_name_ref
            |   empire_id_ref
            |   empire_ships_destroyed
            |   jumps_between
            //|   jumps_between_by_empire_supply
            |   outposts_owned
            |   parts_in_ship_design
            |   part_class_in_ship_design
            |   ship_parts_owned
            |   empire_design_ref
            |   slots_in_hull
            |   slots_in_ship_design
            |   special_added_on_turn
            |   planet_type_difference
            ;

        game_rule.name("GameRule");
        empire_name_ref.name("EmpirePropertyWithName");
        empire_id_ref.name("EmpirePropertyWithID");
        empire_ships_destroyed.name("EmpireShipsDestroyed");
        jumps_between.name("JumpsBetween");
        //jumps_between_by_empire_supply.name("JumpsBetweenByEmpireSupplyConnections");
        outposts_owned.name("OutpostsOwned");
        parts_in_ship_design.name("PartsInShipDesign");
        part_class_in_ship_design.name("PartOfClassInShipDesign");
        part_class_as_int.name("PartClass");
        ship_parts_owned.name("ShipPartsOwned");
        empire_design_ref.name("ShipDesignsDestroyed, ShipDesignsInProduction, ShipDesignsLost, ShipDesignsOwned, ShipDesignsProduced, or ShipDesignsScrapped");
        slots_in_hull.name("SlotsInHull");
        slots_in_ship_design.name("SlotsInShipDesign");
        special_added_on_turn.name("SpecialAddedOnTurn");
        planet_type_difference.name("PlanetTypeDifference");

#if DEBUG_INT_COMPLEX_PARSERS
        debug(game_rule);
        debug(empire_name_ref);
        debug(empire_id_ref);
        debug(empire_ships_destroyed);
        debug(jumps_between);
        //debug(jumps_between_by_empire_supply);
        debug(outposts_owned);
        debug(parts_in_ship_design);
        debug(part_class_in_ship_design);
        debug(ship_parts_owned_by_name);
        debug(ship_parts_owned_by_class);
        debug(empire_design_ref);
        debug(slots_in_hull);
        debug(slots_in_ship_design);
        debug(special_added_on_turn);
        debug(planet_type_difference);
#endif
    }
}
