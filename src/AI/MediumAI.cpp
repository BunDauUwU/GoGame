#include "MediumAI.h"
#include <QElapsedTimer>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

MediumAI ::MediumAI(SearchOptions options)
{
    m_options = options;
}