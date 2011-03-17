/*
 * $Revision: 577 $ $Date: 2011-03-16 17:40:22 -0700 (Wed, 16 Mar 2011) $
 *
 * Copyright by Astos Solutions GmbH, Germany
 *
 * this file is published under the Astos Solutions Free Public License
 * For details on copyright and terms of use see 
 * http://www.astos.de/Astos_Solutions_Free_Public_License.html
 */

#ifndef _VESTA_WORLD_GEOMETRY_H_
#define _VESTA_WORLD_GEOMETRY_H_

#include "Geometry.h"
#include "Material.h"
#include "MapLayer.h"
#include <Eigen/Core>
#include <vector>


namespace vesta
{

class RenderContext;
class TextureMap;
class Atmosphere;
class QuadtreeTile;
class QuadtreeTileAllocator;
class TiledMap;
class PlanetaryRings;
class WorldLayer;

/** WorldGeometry is a Geometry object specialized for rendering
  * spherical (or ellipsoidal) worlds. Optionally, a WorldGeometry
  * object may have a cloud layer, an atmosphere, a ring system,
  * and one or more map layers.
  */
class WorldGeometry : public Geometry
{
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    WorldGeometry();
    virtual ~WorldGeometry();

    void render(RenderContext& rc,
                double clock) const;
    
    float boundingSphereRadius() const;

    virtual float nearPlaneDistance(const Eigen::Vector3f& cameraPosition) const;

    /** Get the lengths of the axes of the globe in kilometers. Note that these are
      * diameters, not radii.
      */
    Eigen::Vector3f ellipsoidAxes() const
    {
        return m_ellipsoidAxes;
    }

    /** Get the maximum radius of the globe in kilometers.
      */
    float maxRadius() const
    {
        return m_ellipsoidAxes.maxCoeff() * 0.5f;
    }

    /** Get the minimum radius of the globe in kilometers.
      */
    float minRadius() const
    {
        return m_ellipsoidAxes.minCoeff() * 0.5f;
    }

    /** Get the polar radius of the globe in kilometers.
      */
    float polarRadius() const
    {
        return m_ellipsoidAxes.z() * 0.5f;
    }

    /** Get the mean radius of the globe in kilometers. This is calculated as the
      * arithmetic mean of the semi-axes of the ellipsoid.
      */
    float meanRadius() const
    {
        return m_ellipsoidAxes.sum() / 6.0f;
    }

    /** Get the mean equatorial radius of the globe in kilometers. This is calculated as the
      * arithmetic mean of the x and y axes of the ellipsoid.
      */
    float meanEquatorialRadius() const
    {
        return (m_ellipsoidAxes.x() + m_ellipsoidAxes.y()) / 4.0f;
    }

    void setSphere(float radius);
    void setSpheroid(float radius, float oblateness);
    void setEllipsoid(const Eigen::Vector3f& axes);

    /** isEllipsoidal always returns true for WorldGeometry.
      */
    virtual bool isEllipsoidal() const { return true; }

    virtual AlignedEllipsoid ellipsoid() const
    {
        return AlignedEllipsoid(m_ellipsoidAxes.cast<double>() * 0.5);
    }

    /** Get the global base texture.
      */
    TextureMap* baseMap() const
    {
        return m_baseMap.ptr();
    }

    void setBaseMap(TextureMap* baseMap);

    TiledMap* tiledMap() const
    {
        return m_baseTiledMap.ptr();
    }

    void setBaseMap(TiledMap* baseMap);

    /** Get the global normal map. The normal map contains normal perturbations
      * derived from a height map.
      */
    TextureMap* normalMap() const
    {
        return m_normalMap.ptr();
    }

    void setNormalMap(TextureMap* normalMap);

    void addLayer(MapLayer* layer);
    void removeLayer(unsigned int index);
    void removeLayer();
    MapLayer* layer(unsigned int index) const;
    unsigned int layerCount() const;

    /** Return true if this globe is self-luminous (e.g. the Sun)
      */
    bool isEmissive() const
    {
        return m_emissive;
    }

    /** Set whether this globe is self-luminous. If true, it
      * will not have any shading applied. Emissive true is the
      * appropriate setting for the Sun. Note that setting emissive
      * to true will *not* make the object a light source.
      */
    void setEmissive(bool emissive)
    {
        m_emissive = emissive;
    }

    Atmosphere* atmosphere() const;

    void setAtmosphere(Atmosphere* atmosphere);

    /** Get the cloud texture map.
      */
    TextureMap* cloudMap() const
    {
        return m_cloudMap.ptr();
    }

    void setCloudMap(TextureMap* cloudMap);

    /** Get the altitude (in km) of the cloud layer above surface of the planet.
      */
    float cloudAltitude() const
    {
        return m_cloudAltitude;
    }

    /** Set the altitude (in km) of the cloud layer above the surface of the planet.
      */
    void setCloudAltitude(float altitude)
    {
        m_cloudAltitude = altitude;
    }

    /** Get the ring system. Returns null if the planet has no rings.
      */
    PlanetaryRings* ringSystem() const
    {
        return m_ringSystem.ptr();
    }

    void setRingSystem(PlanetaryRings* rings);

    /** Get the specular reflectance coefficients. These will be zero (black) for
      * globes that aren't glossy.
      */
    Spectrum specularReflectance() const
    {
        return m_specularReflectance;
    }

    /** Set the specular reflectance coefficients. Specular reflectance can be set
      * to non-zero for worlds that have specular reflections from liquids or smooth
      * ice on their surfaces.
      */
    void setSpecularReflectance(const Spectrum& reflectance)
    {
        m_specularReflectance = reflectance;
    }

    /** Get the specular power. Larger values produce a smaller
      * specular highlight on the reflective portions of the globe, making
      * the surface appear smoother.
      */
    float specularPower() const
    {
        return m_specularPower;
    }

    /** Set the specular power. Larger values produce a smaller
      * specular highlight on the reflective portions of the globe, making
      * the surface appear smoother.
      */
    void setSpecularPower(float specularPower)
    {
        m_specularPower = specularPower;
    }

    typedef std::map<std::string, counted_ptr<WorldLayer> > WorldLayerTable;
    const WorldLayerTable* layers() const
    {
        return &m_layers;
    }

    void setLayer(const std::string& tag, WorldLayer* layer);
    void removeLayer(const std::string& tag);
    WorldLayer* layer(const std::string& tag) const;
    bool hasLayers() const;
    void clearLayers();

protected:
    virtual bool handleRayPick(const Eigen::Vector3d& pickOrigin,
                               const Eigen::Vector3d& pickDirection,
                               double clock,
                               double* distance) const;

private:
    void renderSphere(RenderContext& rc, int subdivisions) const;
    void renderNormalMappedSphere(RenderContext& rc, int subdivisions) const;
    void renderPatch(int subdivisions, const MapLayerBounds& box) const;
    void renderBand(int subdivisions,
                    double latStart,
                    double latEnd,
                    double lonStart,
                    double lonEnd,
                    float tStart,
                    float tEnd) const;

    void initQuadtree(const Eigen::Vector3f& semiAxes, QuadtreeTile** westHemi, QuadtreeTile** eastHemi) const;

private:
    Eigen::Vector3f m_ellipsoidAxes;
    counted_ptr<TextureMap> m_baseMap;
    counted_ptr<TextureMap> m_normalMap;
    counted_ptr<TiledMap> m_baseTiledMap;
    counted_ptr<Material> m_material;
    counted_ptr<Atmosphere> m_atmosphere;
    counted_ptr<PlanetaryRings> m_ringSystem;
    WorldLayerTable m_layers;
    std::vector<counted_ptr<MapLayer> > m_mapLayers;
    bool m_emissive;
    Spectrum m_specularReflectance;
    float m_specularPower;

    counted_ptr<TextureMap> m_cloudMap;
    float m_cloudAltitude;

    QuadtreeTileAllocator* m_tileAllocator;
};

}

#endif // _VESTA_WOLRD_GEOMETRY_H_