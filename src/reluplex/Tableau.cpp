/*********************                                                        */
/*! \file Tableau.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorization.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "ReluplexError.h"
#include "Tableau.h"

#include <cfloat>
#include <string.h>

Tableau::Tableau()
    : _A( NULL )
    , _B( NULL )
    , _AN( NULL )
    , _a( NULL )
    , _d( NULL )
    , _b( NULL )
    , _basisFactorization( NULL )
    , _costFunction( NULL )
    , _basicCosts( NULL )
    , _multipliers( NULL )
    , _basicIndexToVariable( NULL )
    , _nonBasicIndexToVariable( NULL )
    , _variableToIndex( NULL )
    , _nonBasicAtUpper( NULL )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _assignment( NULL )
    , _assignmentStatus( ASSIGNMENT_INVALID )
    , _basicStatus( NULL )
{
}

Tableau::~Tableau()
{
    if ( _A )
    {
        delete[] _A;
        _A = NULL;
    }

    if ( _B )
    {
        delete[] _B;
        _B = NULL;
    }

    if ( _AN )
    {
        delete[] _AN;
        _AN = NULL;
    }

    if ( _d )
    {
        delete[] _d;
        _d = NULL;
    }

    if ( _b )
    {
        delete[] _b;
        _b = NULL;
    }

    if ( _costFunction )
    {
        delete[] _costFunction;
        _costFunction = NULL;
    }

    if ( _basicCosts )
    {
        delete[] _basicCosts;
        _basicCosts = NULL;
    }

    if ( _multipliers )
    {
        delete[] _multipliers;
        _multipliers = NULL;
    }

    if ( _basicIndexToVariable )
    {
        delete[] _basicIndexToVariable;
        _basicIndexToVariable = NULL;
    }

    if ( _variableToIndex )
    {
        delete[] _variableToIndex;
        _variableToIndex = NULL;
    }

    if ( _nonBasicIndexToVariable )
    {
        delete[] _nonBasicIndexToVariable;
        _nonBasicIndexToVariable = NULL;
    }

    if ( _nonBasicAtUpper )
    {
        delete[] _nonBasicAtUpper;
        _nonBasicAtUpper = NULL;
    }

    if ( _lowerBounds )
    {
        delete[] _lowerBounds;
        _lowerBounds = NULL;
    }

    if ( _upperBounds )
    {
        delete[] _upperBounds;
        _upperBounds = NULL;
    }

    if ( _assignment )
    {
        delete[] _assignment;
        _assignment = NULL;
    }

    if ( _basicStatus )
    {
        delete[] _basicStatus;
        _basicStatus = NULL;
    }

    if ( _basisFactorization )
    {
        delete _basisFactorization;
        _basisFactorization = NULL;
    }
}

void Tableau::setDimensions( unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    _A = new double[n*m];
    if ( !_A )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::A" );

    _B = new double[m*m];
    if ( !_B )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::B" );

    _AN = new double[m * (n-m)];
    if ( !_AN )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::AN" );

    _d = new double[m];
    if ( !_d )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::d" );

    _b = new double[m];
    if ( !_b )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::b" );

    _costFunction = new double[n-m];
    if ( !_costFunction )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::costFunction" );

    _basicCosts = new double[m];
    if ( !_basicCosts )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basicCosts" );

    _multipliers = new double[m];
    if ( !_multipliers )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::multipliers" );

    _basicIndexToVariable = new unsigned[m];
    if ( !_basicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basicIndexToVariable" );

    _variableToIndex = new unsigned[n];
    if ( !_variableToIndex )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::variableToIndex" );

    _nonBasicIndexToVariable = new unsigned[n-m];
    if ( !_nonBasicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::nonBasicIndexToVariable" );

    _nonBasicAtUpper = new bool[n-m];
    if ( !_nonBasicAtUpper )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::nonBasicValues" );

    _lowerBounds = new double[n];
    if ( !_lowerBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::lowerBounds" );

    _upperBounds = new double[n];
    if ( !_upperBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::upperBounds" );

    _assignment = new double[m];
    if ( !_assignment )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::assignment" );

    _basicStatus = new unsigned[m];
    if ( !_basicStatus )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basicStatus" );

    _basisFactorization = new BasisFactorization( _m );
    if ( !_basisFactorization )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basisFactorization" );
}

void Tableau::setEntryValue( unsigned row, unsigned column, double value )
{
    _A[(column * _m) + row] = value;
}

void Tableau::initializeBasis( const Set<unsigned> &basicVariables )
{
    unsigned basicIndex = 0;
    unsigned nonBasicIndex = 0;

    _basicVariables = basicVariables;

    // Assign variable indices, grab basic columns from A into B
    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( basicVariables.exists( i ) )
        {
            _basicIndexToVariable[basicIndex] = i;
            _variableToIndex[i] = basicIndex;
            memcpy( _B + basicIndex * _m, _A + i * _m, sizeof(double) * _m );
            ++basicIndex;
        }
        else
        {
            _nonBasicIndexToVariable[nonBasicIndex] = i;
            _variableToIndex[i] = nonBasicIndex;
            memcpy( _AN + nonBasicIndex * _m, _A + i * _m, sizeof(double) * _m );
            ++nonBasicIndex;
        }
    }
    ASSERT( basicIndex + nonBasicIndex == _n );

    // Set non-basics to lower bounds
    std::fill( _nonBasicAtUpper, _nonBasicAtUpper + _n - _m, false );

    // Recompute assignment
    computeAssignment();
}

void Tableau::computeAssignment()
{
    if ( _assignmentStatus == ASSIGNMENT_VALID )
        return;

    /*
      The basic assignment is given by the formula:

       xB = inv(B) * b - inv(B) * AN * xN
          = inv(B) * ( b - AN * xN )
                       -----------
                            y

       where B is the basis matrix, AN is the non-basis matrix, xN are
       the value of the non basic variables and b is the original
       right hand side.

       We first compute y, and then do an FTRAN pass to solve B*xB = y
    */

    double *y = new double[_m];
    memcpy( y, _b, sizeof(double) * _m );

    // Compute a linear combination of the columns of AN
    double *ANColumn;
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned var = _nonBasicIndexToVariable[i];
        double value = _nonBasicAtUpper[i] ? _upperBounds[var] : _lowerBounds[var];
        ANColumn = _AN + ( i * _m );
        for ( unsigned j = 0; j < _m; ++j )
            y[j] -= ANColumn[j] * value;
    }

    // Solve B*xB = y by performing a forward transformation
    _basisFactorization->forwardTransformation( y, _assignment );

    delete[] y;

    computeBasicStatus();
    _assignmentStatus = ASSIGNMENT_VALID;
}

void Tableau::computeBasicStatus()
{
    for ( unsigned i = 0; i < _m; ++i )
        computeBasicStatus( i );
}

void Tableau::computeBasicStatus( unsigned basic )
{
    double ub = _upperBounds[_basicIndexToVariable[basic]];
    double lb = _lowerBounds[_basicIndexToVariable[basic]];
    double value = _assignment[basic];

    if ( FloatUtils::gt( value , ub ) )
        _basicStatus[basic] = Tableau::ABOVE_UB;
    else if ( FloatUtils::lt( value , lb ) )
        _basicStatus[basic] = Tableau::BELOW_LB;
    else if ( FloatUtils::areEqual( ub, value ) )
        _basicStatus[basic] = Tableau::AT_UB;
    else if ( FloatUtils::areEqual( lb, value ) )
        _basicStatus[basic] = Tableau::AT_LB;
    else
        _basicStatus[basic] = Tableau::BETWEEN;
}

void Tableau::setLowerBound( unsigned variable, double value )
{
    ASSERT( variable < _n );
    _lowerBounds[variable] = value;
}

void Tableau::setUpperBound( unsigned variable, double value )
{
    ASSERT( variable < _n );
    _upperBounds[variable] = value;
}

double Tableau::getValue( unsigned variable )
{
    if ( !_basicVariables.exists( variable ) )
    {
        // The values of non-basics can be extracted even if the
        // assignment is invalid
        unsigned index = _variableToIndex[variable];
        return _nonBasicAtUpper[index] ? _upperBounds[variable] : _lowerBounds[variable];
    }

    // Values of basic variabels require valid assignments
    // TODO: maybe we should compute just that one variable?
    if ( _assignmentStatus != ASSIGNMENT_VALID )
        computeAssignment();

    return _assignment[_variableToIndex[variable]];
}

void Tableau::setRightHandSide( const double *b )
{
    memcpy( _b, b, sizeof(double) * _m );
}

const double *Tableau::getCostFunction()
{
    computeCostFunction();
    return _costFunction;
}

bool Tableau::basicOutOfBounds( unsigned basic ) const
{
    return basicTooHigh( basic ) || basicTooLow( basic );
}

bool Tableau::basicTooLow( unsigned basic ) const
{
    return _basicStatus[basic] == Tableau::BELOW_LB;
}

bool Tableau::basicTooHigh( unsigned basic ) const
{
    return _basicStatus[basic] == Tableau::ABOVE_UB;
}

bool Tableau::existsBasicOutOfBounds() const
{
    for ( unsigned i = 0; i < _m; ++i )
        if ( basicOutOfBounds( i ) )
            return true;

    return false;
}

void Tableau::computeCostFunction()
{
    /*
      The cost function is computed in three steps:

      1. Compute the basic costs c.
         These costs indicate whether a basic variable's row in
         the tableau should be added as is (variable too great;
         cost = 1), should be added negatively (variable is too
         small; cost = -1), or should be ignored (variable
         within bounds; cost = 0).

      2. Compute the multipliers p.
         p = c' * inv(B)
         This is solved by invoking BTRAN for pB = c'

      3. Compute the non-basic (reduced) costs.
         These are given by -p * AN

      Comment: the correctness follows from the fact that

      xB = inv(B)(b - AN xN)

      we ignore b because the constants don't matter for the cost
      function, and we omit xN because we want the function and not an
      evaluation thereof on a specific point.
     */

    // Step 1: compute basic costs
    computeBasicCosts();

    // Step 2: compute the multipliers
    computeMultipliers();

    // Step 3: compute reduced costs
    computeReducedCosts();
}

void Tableau::computeBasicCosts()
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( basicTooLow( i ) )
            _basicCosts[i] = -1;
        else if ( basicTooHigh( i ) )
            _basicCosts[i] = 1;
        else
            _basicCosts[i] = 0;
    }
}

void Tableau::computeMultipliers()
{
    _basisFactorization->backwardTransformation( _basicCosts, _multipliers );
}

void Tableau::computeReducedCosts()
{
    double *ANColumn;
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        ANColumn = _AN + ( i * _m );

        _costFunction[i] = 0;
        for ( unsigned j = 0; j < _m; ++j )
            _costFunction[i] -= ( _multipliers[j] * ANColumn[j] );
    }
}

unsigned Tableau::getBasicStatus( unsigned basic )
{
    return _basicStatus[_variableToIndex[basic]];
}

bool Tableau::pickEnteringVariable()
{
    List<unsigned> candidates;
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        if ( eligibleForEntry( i ) )
            candidates.append( i );
    }

    // TODO: is it really the case that no candidates -->
    // infeasibility? Maybe they canceled out or something?
    if ( candidates.empty() )
        return false;

    // Dantzig's rule
    auto candidate = candidates.begin();
    unsigned maxIndex = *candidate;
    double maxValue = FloatUtils::abs( _costFunction[maxIndex] );
    ++candidate;

    while ( candidate != candidates.end() )
    {
        double contenderValue = FloatUtils::abs( _costFunction[*candidate] );
        if ( FloatUtils::gt( contenderValue, maxValue ) )
        {
            maxIndex = *candidate;
            maxValue = contenderValue;
        }
        ++candidate;
    }

    _enteringVariable = maxIndex;
    return true;
}

bool Tableau::eligibleForEntry( unsigned nonBasic )
{
    // A non-basic variable is eligible for entry if one of the two
    //   conditions holds:
    //
    //   1. It has a negative coefficient in the cost function and it
    //      can increase
    //   2. It has a positive coefficient in the cost function and it
    //      can decrease

    if ( FloatUtils::isZero( _costFunction[nonBasic] ) )
        return false;

    bool positive = FloatUtils::isPositive( _costFunction[nonBasic] );

    return
        ( positive && _nonBasicAtUpper[nonBasic] ) ||
        ( !positive && !_nonBasicAtUpper[nonBasic] );
}

unsigned Tableau::getEnteringVariable() const
{
    return _nonBasicIndexToVariable[_enteringVariable];
}

void Tableau::performPivot()
{
    // Any kind of pivot invalidates the assignment
    // TODO: we don't really need to invalidate, can update the basis
    // vars based on _d
    _assignmentStatus = ASSIGNMENT_INVALID;

    if ( _leavingVariable == _m )
    {
        // The entering variable is simply switching to its opposite bound.
        _nonBasicAtUpper[_enteringVariable] = !_nonBasicAtUpper[_enteringVariable];
        return;
    }

    unsigned currentBasic = _basicIndexToVariable[_leavingVariable];
    unsigned currentNonBasic = _nonBasicIndexToVariable[_enteringVariable];

    // Update the database
    _basicVariables.insert( currentNonBasic );
    _basicVariables.erase( currentBasic );

    // Adjust the tableau indexing
    _basicIndexToVariable[_leavingVariable] = currentNonBasic;
    _nonBasicIndexToVariable[_enteringVariable] = currentBasic;
    _variableToIndex[currentBasic] = _enteringVariable;
    _variableToIndex[currentNonBasic] = _leavingVariable;

    // Update value of the old basic (now non-basic) variable
    _nonBasicAtUpper[_enteringVariable] = _leavingVariableIncreases;

    // Update the basis factorization. The column corresponding to the
    // leaving variable is the one that has changed
    _basisFactorization->pushEtaMatrix( _leavingVariable, _d );
}

double Tableau::ratioConstraintPerBasic( unsigned basicIndex, double coefficient, bool decrease )
{
    unsigned basic = _basicIndexToVariable[basicIndex];
    double ratio;

    ASSERT( !FloatUtils::isZero( coefficient ) );

    if ( ( FloatUtils::isPositive( coefficient ) && decrease ) ||
         ( FloatUtils::isNegative( coefficient ) && !decrease ) )
    {
        // Basic variable is decreasing
        double maxChange;

        if ( _basicStatus[basicIndex] == BasicStatus::ABOVE_UB )
        {
            // Maximal change: hitting the upper bound
            maxChange = _upperBounds[basic] - _assignment[basicIndex];
        }
        else if ( _basicStatus[basicIndex] == BasicStatus::BETWEEN )
        {
            // Maximal change: hitting the lower bound
            maxChange = _lowerBounds[basic] - _assignment[basicIndex];
        }
        else if ( ( _basicStatus[basicIndex] == BasicStatus::AT_UB ) ||
                  ( _basicStatus[basicIndex] == BasicStatus::AT_LB ) )
        {
            // Variable is pressed against a bound - can't change!
            maxChange = 0;
        }
        else
        {
            // Variable is below its lower bound, no constraint here
            maxChange = - DBL_MAX - _assignment[basicIndex];
        }

        ratio = maxChange / coefficient;
    }
    else if ( ( FloatUtils::isPositive( coefficient ) && !decrease ) ||
              ( FloatUtils::isNegative( coefficient ) && decrease ) )

    {
        // Basic variable is increasing
        double maxChange;

        if ( _basicStatus[basicIndex] == BasicStatus::BELOW_LB )
        {
            // Maximal change: hitting the lower bound
            maxChange = _lowerBounds[basic] - _assignment[basicIndex];
        }
        else if ( _basicStatus[basicIndex] == BasicStatus::BETWEEN )
        {
            // Maximal change: hitting the upper bound
            maxChange = _upperBounds[basic] - _assignment[basicIndex];
        }
        else if ( ( _basicStatus[basicIndex] == BasicStatus::AT_UB ) ||
                  ( _basicStatus[basicIndex] == BasicStatus::AT_LB ) )
        {
            // Variable is pressed against a bound - can't change!
            maxChange = 0;
        }
        else
        {
            // Variable is above its upper bound, no constraint here
            maxChange = DBL_MAX - _assignment[basicIndex];
        }

        ratio = maxChange / coefficient;
    }
    else
    {
        ASSERT( false );
    }

    return ratio;
}

void Tableau::pickLeavingVariable( double *d )
{
    ASSERT( !FloatUtils::isZero( _costFunction[_enteringVariable] ) );
    bool decrease = FloatUtils::isPositive( _costFunction[_enteringVariable] );

    DEBUG({
        if ( decrease )
        {
            ASSERTM( _nonBasicAtUpper[_enteringVariable],
                     "Error! Entering variable needs to decreased but is at its lower bound" );
        }
        else
        {
            ASSERTM( !_nonBasicAtUpper[_enteringVariable],
                     "Error! Entering variable needs to decreased but is at its upper bound" );
        }
        });

    double lb = _lowerBounds[_enteringVariable];
    double ub = _upperBounds[_enteringVariable];

    // A marker to show that no leaving variable has been selected
    _leavingVariable = _m;

    // TODO: assuming all coefficient in B are 1

    if ( decrease )
    {
        // The maximum amount by which the entering variable can
        // decrease, as determined by its bounds. This is a negative
        // value.
        _changeRatio = lb - ub;

        // Iterate over the basics that depend on the entering
        // variable and see if any of them imposes a tighter
        // constraint.
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( !FloatUtils::isZero( d[i] ) )
            {
                double ratio = ratioConstraintPerBasic( i, d[i], decrease );
                if ( ratio > _changeRatio )
                {
                    _changeRatio = ratio;
                    _leavingVariable = i;
                }
            }
        }

        _leavingVariableIncreases = FloatUtils::isNegative( d[_leavingVariable] );
    }
    else
    {
        // The maximum amount by which the entering variable can
        // increase, as determined by its bounds. This is a positive
        // value.
        _changeRatio = ub - lb;

        // Iterate over the basics that depend on the entering
        // variable and see if any of them imposes a tighter
        // constraint.
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( !FloatUtils::isZero( d[i] ) )
            {
                double ratio = ratioConstraintPerBasic( i, d[i], decrease );
                if ( ratio < _changeRatio )
                {
                    _changeRatio = ratio;
                    _leavingVariable = i;
                }
            }
        }

        _leavingVariableIncreases = FloatUtils::isPositive( d[_leavingVariable] );
    }
}

unsigned Tableau::getLeavingVariable() const
{
    if ( _leavingVariable == _m )
        return _nonBasicIndexToVariable[_enteringVariable];

    return _basicIndexToVariable[_leavingVariable];
}

double Tableau::getChangeRatio() const
{
    return _changeRatio;
}

void Tableau::computeD()
{
    // _a gets the entering variable's column in AN
    _a = _AN + ( _enteringVariable * _m );

    // Compute d = inv(B) * a using the basis factorization
    _basisFactorization->forwardTransformation( _a, _d );
}

bool Tableau::isBasic( unsigned variable ) const
{
    return _basicVariables.exists( variable );
}

void Tableau::dump() const
{
    printf( "\nDumping A:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _n; ++j )
        {
            printf( "%5.1lf ", _A[j * _m + i] );
        }
        printf( "\n" );
    }

    printf( "\nDumping B:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _m; ++j )
        {
            printf( "%5.1lf ", _B[j * _m + i] );
        }
        printf( "\n" );
    }

    printf( "\nDumping AN:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _n - _m; ++j )
        {
            printf( "%5.1lf ", _AN[j * _m + i] );
        }
        printf( "\n" );
    }
}

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
