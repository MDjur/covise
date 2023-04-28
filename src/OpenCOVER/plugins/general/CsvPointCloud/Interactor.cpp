#include "Interactor.h"
#include <sstream>

std::string createColorMapString(const std::string& name, const covise::ColorMap &cm, float min, float max)
{
    int v = 0;
    std::stringstream ss;
    ss << "OBJ_Name\n"
       << name +"\n"
       << min << std::endl << max << std::endl << (int)cm.samplingPoints.size() << std::endl << v << std::endl;
    for (size_t i = 0; i < cm.samplingPoints.size(); i++)
    {
        ss << cm.r[i] << std::endl
           << cm.g[i] << std::endl
           << cm.b[i] << std::endl
           << cm.a[i] << std::endl;
    }
    return ss.str();
}

void CsvInteractor::setVectorParam(const char *name, int numElem, float *field)
{
    if(std::string(name) == "MinMax" && numElem == 2)
    {
        m_minSlider = field[0];
        m_maxSlider = field[1];
    }
}

void CsvInteractor::setColorMap(const covise::ColorMap &cm)
{
    m_colorMap = cm;
    m_numColorSteps = cm.samplingPoints.size();
}

covise::ColorMap CsvInteractor::getColorMap() const
{
    return covise::interpolateColorMap(m_colorMap, m_numColorSteps);
}

void CsvInteractor::setName(const std::string& name)
{
    m_name = name;
}

void CsvInteractor::setMinMax(float min, float max)
{
    m_min = min;
    m_max = max;
    m_minSlider = min;
    m_maxSlider = max;
}

void CsvInteractor::setScalarParam(const char *name, int val)
{
    if(std::string(name) == "steps")
    {
        m_numColorSteps = val;
    }
}

const char *CsvInteractor::getString(unsigned int i) const
{
    static std::string s;
    s = createColorMapString(m_name, covise::interpolateColorMap(m_colorMap, m_numColorSteps), m_minSlider, m_maxSlider);
    return s.c_str();
}

int CsvInteractor::getIntScalarParam(const std::string &paraName, int &value) const 
{
    if(paraName == "steps")
    {
        value = m_numColorSteps;
        return -1;
    }
    return 0;
}

int CsvInteractor::getFloatScalarParam(const std::string &paraName, float &value) const
{
    if(paraName == "min")
    {
        value = m_minSlider;
        return -1;
    }
    else if(paraName == "max")
    {
        value = m_maxSlider;
        return -1;
    }
    return 0;
}

int CsvInteractor::getIntSliderParam(const std::string &paraName, int &min, int &max, int &val) const
{
    if(paraName == "steps")
    {
        min = 2;
        max = 256;
        val = m_numColorSteps;
    }
    return 0;
}

int CsvInteractor::getFloatSliderParam(const std::string &paraName, float &min, float &max, float &val) const
{
    if(paraName == "min")
    {
        val = m_minSlider;
        float diff = (m_max - m_min) / 2;
        min = m_min - diff;
        max = m_min + diff;
    }

    if(paraName == "max")
    {
        val = m_maxSlider;
        float diff = (m_max - m_min) / 2;
        min = m_max - diff;
        max = m_max + diff;
    }
    return 0;
}
