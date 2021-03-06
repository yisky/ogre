/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
#include "OgreStableHeaders.h"

#include "OgreTextureUnitState.h"
#include "OgreControllerManager.h"

namespace Ogre {

    //-----------------------------------------------------------------------
    TextureUnitState::TextureUnitState(Pass* parent)
        : mCurrentFrame(0)
        , mAnimDuration(0)
        , mCubic(false)
        , mTextureCoordSetIndex(0)
        , mBorderColour(ColourValue::Black)
        , mTextureLoadFailed(false)
        , mGamma(1)
        , mRecalcTexMatrix(false)
        , mUMod(0)
        , mVMod(0)
        , mUScale(1)
        , mVScale(1)
        , mRotate(0)
        , mTexModMatrix(Matrix4::IDENTITY)
        , mMinFilter(FO_LINEAR)
        , mMagFilter(FO_LINEAR)
        , mMipFilter(FO_POINT)
        , mCompareEnabled(false)
        , mCompareFunc(CMPF_GREATER_EQUAL)
        , mMaxAniso(MaterialManager::getSingleton().getDefaultAnisotropy())
        , mMipmapBias(0)
        , mIsDefaultAniso(true)
        , mIsDefaultFiltering(true)
        , mBindingType(BT_FRAGMENT)
        , mContentType(CONTENT_NAMED)
        , mParent(parent)
        , mAnimController(0)
    {
        mColourBlendMode.blendType = LBT_COLOUR;
        mAlphaBlendMode.operation = LBX_MODULATE;
        mAlphaBlendMode.blendType = LBT_ALPHA;
        mAlphaBlendMode.source1 = LBS_TEXTURE;
        mAlphaBlendMode.source2 = LBS_CURRENT;
        setColourOperation(LBO_MODULATE);
        setTextureAddressingMode(TAM_WRAP);

        if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
        {
            mParent->_dirtyHash();
        }

    }

    //-----------------------------------------------------------------------
    TextureUnitState::TextureUnitState(Pass* parent, const TextureUnitState& oth )
    {
        mParent = parent;
        mAnimController = 0;
        *this = oth;
    }

    //-----------------------------------------------------------------------
    TextureUnitState::TextureUnitState( Pass* parent, const String& texName, unsigned int texCoordSet)
        : mCurrentFrame(0)
        , mAnimDuration(0)
        , mCubic(false)
        , mTextureCoordSetIndex(0)
        , mBorderColour(ColourValue::Black)
        , mTextureLoadFailed(false)
        , mGamma(1)
        , mRecalcTexMatrix(false)
        , mUMod(0)
        , mVMod(0)
        , mUScale(1)
        , mVScale(1)
        , mRotate(0)
        , mTexModMatrix(Matrix4::IDENTITY)
        , mMinFilter(FO_LINEAR)
        , mMagFilter(FO_LINEAR)
        , mMipFilter(FO_POINT)
        , mMaxAniso(MaterialManager::getSingleton().getDefaultAnisotropy())
        , mMipmapBias(0)
        , mIsDefaultAniso(true)
        , mIsDefaultFiltering(true)
        , mBindingType(BT_FRAGMENT)
        , mContentType(CONTENT_NAMED)
        , mParent(parent)
        , mAnimController(0)
    {
        mColourBlendMode.blendType = LBT_COLOUR;
        mAlphaBlendMode.operation = LBX_MODULATE;
        mAlphaBlendMode.blendType = LBT_ALPHA;
        mAlphaBlendMode.source1 = LBS_TEXTURE;
        mAlphaBlendMode.source2 = LBS_CURRENT;
        setColourOperation(LBO_MODULATE);
        setTextureAddressingMode(TAM_WRAP);

        setTextureName(texName);
        setTextureCoordSet(texCoordSet);

        if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
        {
            mParent->_dirtyHash();
        }

    }
    //-----------------------------------------------------------------------
    TextureUnitState::~TextureUnitState()
    {
        // Unload ensure all controllers destroyed
        _unload();
    }
    //-----------------------------------------------------------------------
    TextureUnitState & TextureUnitState::operator = ( 
        const TextureUnitState &oth )
    {
        assert(mAnimController == 0);
        removeAllEffects();

        // copy basic members (int's, real's)
        memcpy( (uchar*)this, &oth, (const uchar *)(&oth.mFramePtrs) - (const uchar *)(&oth) );
        // copy complex members
        mFramePtrs = oth.mFramePtrs;
        mName    = oth.mName;
        mEffects = oth.mEffects;

        mTextureNameAlias = oth.mTextureNameAlias;
        mCompositorRefName = oth.mCompositorRefName;
        mCompositorRefTexName = oth.mCompositorRefTexName;
        // Can't sharing controllers with other TUS, reset to null to avoid potential bug.
        for (EffectMap::iterator j = mEffects.begin(); j != mEffects.end(); ++j)
        {
            j->second.controller = 0;
        }

        // Load immediately if Material loaded
        if (isLoaded())
        {
            _load();
        }

        // Tell parent to recalculate hash
        if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
        {
            mParent->_dirtyHash();
        }

        return *this;
    }
    //-----------------------------------------------------------------------
    const String& TextureUnitState::getTextureName(void) const
    {
        // Return name of current frame
        if (mCurrentFrame < mFramePtrs.size())
            return mFramePtrs[mCurrentFrame]->getName();
        else
            return BLANKSTRING;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureName( const String& name, TextureType texType)
    {
        TexturePtr tex = retrieveTexture(name);

        if(!tex)
            return;

        tex->setTextureType(texType);
        setTexture(tex);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTexture( const TexturePtr& texPtr)
    {
        if (!texPtr)
        {
            OGRE_EXCEPT(Exception::ERR_ITEM_NOT_FOUND,
                "Texture Pointer is empty.",
                "TextureUnitState::setTexture");
        }

        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        if (texPtr->getTextureType() == TEX_TYPE_CUBE_MAP)
        {
            // delegate to cubic texture implementation
            setCubicTexture(&texPtr, true);
        }
        else
        {
            mFramePtrs.resize(1);
            mFramePtrs[0] = texPtr;

            mCurrentFrame = 0;
            mCubic = false;

            // Load immediately ?
            if (isLoaded())
            {
                _load(); // reload
            }
            // Tell parent to recalculate hash
            if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
            {
                mParent->_dirtyHash();
            }
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setBindingType(TextureUnitState::BindingType bt)
    {
        mBindingType = bt;

    }
    //-----------------------------------------------------------------------
    TextureUnitState::BindingType TextureUnitState::getBindingType(void) const
    {
        return mBindingType;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setContentType(TextureUnitState::ContentType ct)
    {
        mContentType = ct;
    }
    //-----------------------------------------------------------------------
    TextureUnitState::ContentType TextureUnitState::getContentType(void) const
    {
        return mContentType;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setCubicTextureName( const String& name, bool forUVW)
    {
        if (forUVW)
        {
            setCubicTextureName(&name, forUVW);
        }
        else
        {
            String ext;
            String baseName;
            StringUtil::splitBaseFilename(name, baseName, ext);
            ext = "."+ext;

            String fullNames[6];
            static const char* suffixes[6] = {"_fr", "_bk", "_lf", "_rt", "_up", "_dn"};
            for (int i = 0; i < 6; ++i)
            {
                fullNames[i] = baseName + suffixes[i] + ext;
            }

            setCubicTextureName(fullNames, forUVW);
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setCubicTextureName(const String* const names, bool forUVW)
    {
        mFramePtrs.resize(forUVW ? 1 : 6);
        for (unsigned int i = 0; i < mFramePtrs.size(); ++i)
        {
            mFramePtrs[i] = retrieveTexture(names[i]);
            mFramePtrs[i]->setTextureType(forUVW ? TEX_TYPE_CUBE_MAP : TEX_TYPE_2D);
        }
        setCubicTexture(&mFramePtrs[0], forUVW);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setCubicTexture( const TexturePtr* const texPtrs, bool forUVW )
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;
        mFramePtrs.resize(forUVW ? 1 : 6);
        mAnimDuration = 0;
        mCurrentFrame = 0;
        mCubic = true;

        for (unsigned int i = 0; i < mFramePtrs.size(); ++i)
        {
            mFramePtrs[i] = texPtrs[i];
        }
        // Tell parent we need recompiling, will cause reload too
        mParent->_notifyNeedsRecompile();
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::isCubic(void) const
    {
        return mCubic;
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::is3D(void) const
    {
        return getTextureType() == TEX_TYPE_CUBE_MAP;
    }
    //-----------------------------------------------------------------------
    TextureType TextureUnitState::getTextureType(void) const
    {
        return mFramePtrs.empty() ? TEX_TYPE_2D : mFramePtrs[0]->getTextureType();
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::setFrameTextureName(const String& name, unsigned int frameNumber)
    {
        mTextureLoadFailed = false;
        if (frameNumber < mFramePtrs.size())
        {
            mFramePtrs[frameNumber] = retrieveTexture(name);

            if (isLoaded())
            {
                _load(); // reload
            }
            // Tell parent to recalculate hash
            if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
            {
                mParent->_dirtyHash();
            }
        }
        else // raise exception for frameNumber out of bounds
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "frameNumber parameter value exceeds number of stored frames.",
                "TextureUnitState::setFrameTextureName");
        }
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::addFrameTextureName(const String& name)
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        mFramePtrs.push_back(retrieveTexture(name));

        // Load immediately if Material loaded
        if (isLoaded())
        {
            _load();
        }
        // Tell parent to recalculate hash
        if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
        {
            mParent->_dirtyHash();
        }
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::deleteFrameTextureName(const size_t frameNumber)
    {
        mTextureLoadFailed = false;
        if (frameNumber < mFramePtrs.size())
        {
            mFramePtrs.erase(mFramePtrs.begin() + frameNumber);

            if (isLoaded())
            {
                _load();
            }
            // Tell parent to recalculate hash
            if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
            {
                mParent->_dirtyHash();
            }
        }
        else
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "frameNumber parameter value exceeds number of stored frames.",
                "TextureUnitState::deleteFrameTextureName");
        }
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::setAnimatedTextureName( const String& name, unsigned int numFrames, Real duration)
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        String ext;
        String baseName;

        size_t pos = name.find_last_of('.');
        baseName = name.substr(0, pos);
        ext = name.substr(pos);

        // resize pointers, but don't populate until needed
        mFramePtrs.resize(numFrames);
        mAnimDuration = duration;
        mCurrentFrame = 0;
        mCubic = false;

        for (unsigned int i = 0; i < mFramePtrs.size(); ++i)
        {
            StringStream str;
            str << baseName << "_" << i << ext;
            mFramePtrs[i] = retrieveTexture(str.str());
        }

        // Load immediately if Material loaded
        if (isLoaded())
        {
            _load();
        }
        // Tell parent to recalculate hash
        if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
        {
            mParent->_dirtyHash();
        }

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setAnimatedTextureName(const String* const names, unsigned int numFrames, Real duration)
    {
        setContentType(CONTENT_NAMED);
        mTextureLoadFailed = false;

        // resize pointers, but don't populate until needed
        mFramePtrs.resize(numFrames);
        mAnimDuration = duration;
        mCurrentFrame = 0;
        mCubic = false;

        for (unsigned int i = 0; i < mFramePtrs.size(); ++i)
        {
            mFramePtrs[i] = retrieveTexture(names[i]);
        }

        // Load immediately if Material loaded
        if (isLoaded())
        {
            _load();
        }
        // Tell parent to recalculate hash
        if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
        {
            mParent->_dirtyHash();
        }
    }
    //-----------------------------------------------------------------------
    std::pair< size_t, size_t > TextureUnitState::getTextureDimensions( unsigned int frame ) const
    {
        
        TexturePtr tex = _getTexturePtr(frame);
        if (!tex)
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND, "Could not find texture " + StringConverter::toString(frame),
            "TextureUnitState::getTextureDimensions" );

        return std::pair< size_t, size_t >( tex->getWidth(), tex->getHeight() );
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setCurrentFrame(unsigned int frameNumber)
    {
        if (frameNumber < mFramePtrs.size())
        {
            mCurrentFrame = frameNumber;
            // this will affect the hash
            if( Pass::getHashFunction() == Pass::getBuiltinHashFunction( Pass::MIN_TEXTURE_CHANGE ) )
            {
                mParent->_dirtyHash();
            }
        }
        else
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "frameNumber parameter value exceeds number of stored frames.",
                "TextureUnitState::setCurrentFrame");
        }

    }
    //-----------------------------------------------------------------------
    unsigned int TextureUnitState::getCurrentFrame(void) const
    {
        return mCurrentFrame;
    }
    //-----------------------------------------------------------------------
    unsigned int TextureUnitState::getNumFrames(void) const
    {
        return (unsigned int)mFramePtrs.size();
    }
    //-----------------------------------------------------------------------
    const String& TextureUnitState::getFrameTextureName(unsigned int frameNumber) const
    {
        if (frameNumber >= mFramePtrs.size())
        {
            OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "frameNumber parameter value exceeds number of stored frames.",
                "TextureUnitState::getFrameTextureName");
        }

        return mFramePtrs[frameNumber]->getName();
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setDesiredFormat(PixelFormat desiredFormat)
    {
        for(auto& frame : mFramePtrs)
            frame->setFormat(desiredFormat);
    }
    //-----------------------------------------------------------------------
    PixelFormat TextureUnitState::getDesiredFormat(void) const
    {
        return mFramePtrs.empty() ? PF_UNKNOWN : mFramePtrs[0]->getDesiredFormat();
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setNumMipmaps(int numMipmaps)
    {
        for (auto& frame : mFramePtrs)
            frame->setNumMipmaps(numMipmaps == MIP_DEFAULT
                                     ? TextureManager::getSingleton().getDefaultNumMipmaps()
                                     : numMipmaps);
    }
    //-----------------------------------------------------------------------
    int TextureUnitState::getNumMipmaps(void) const
    {
        return mFramePtrs.empty() ? int(MIP_DEFAULT) : mFramePtrs[0]->getNumMipmaps();
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setIsAlpha(bool isAlpha)
    {
        for(auto& frame : mFramePtrs)
            frame->setTreatLuminanceAsAlpha(isAlpha);
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::getIsAlpha(void) const
    {
        return !mFramePtrs.empty() && mFramePtrs[0]->getTreatLuminanceAsAlpha();
    }
    float TextureUnitState::getGamma() const
    {
        return mFramePtrs.empty() ? 1.0f : mFramePtrs[0]->getGamma();
    }
    void TextureUnitState::setGamma(float gamma)
    {
        for(auto& frame : mFramePtrs)
            frame->setGamma(gamma);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setHardwareGammaEnabled(bool g)
    {
        for(auto& frame : mFramePtrs)
            frame->setHardwareGammaEnabled(g);
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::isHardwareGammaEnabled() const
    {
        return !mFramePtrs.empty() && mFramePtrs[0]->isHardwareGammaEnabled();
    }
    //-----------------------------------------------------------------------
    unsigned int TextureUnitState::getTextureCoordSet(void) const
    {
        return mTextureCoordSetIndex;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureCoordSet(unsigned int set)
    {
        mTextureCoordSetIndex = set;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setColourOperationEx(LayerBlendOperationEx op,
        LayerBlendSource source1,
        LayerBlendSource source2,
        const ColourValue& arg1,
        const ColourValue& arg2,
        Real manualBlend)
    {
        mColourBlendMode.operation = op;
        mColourBlendMode.source1 = source1;
        mColourBlendMode.source2 = source2;
        mColourBlendMode.colourArg1 = arg1;
        mColourBlendMode.colourArg2 = arg2;
        mColourBlendMode.factor = manualBlend;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setColourOperation(LayerBlendOperation op)
    {
        // Set up the multitexture and multipass blending operations
        switch (op)
        {
        case LBO_REPLACE:
            setColourOperationEx(LBX_SOURCE1, LBS_TEXTURE, LBS_CURRENT);
            setColourOpMultipassFallback(SBF_ONE, SBF_ZERO);
            break;
        case LBO_ADD:
            setColourOperationEx(LBX_ADD, LBS_TEXTURE, LBS_CURRENT);
            setColourOpMultipassFallback(SBF_ONE, SBF_ONE);
            break;
        case LBO_MODULATE:
            setColourOperationEx(LBX_MODULATE, LBS_TEXTURE, LBS_CURRENT);
            setColourOpMultipassFallback(SBF_DEST_COLOUR, SBF_ZERO);
            break;
        case LBO_ALPHA_BLEND:
            setColourOperationEx(LBX_BLEND_TEXTURE_ALPHA, LBS_TEXTURE, LBS_CURRENT);
            setColourOpMultipassFallback(SBF_SOURCE_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA);
            break;
        }


    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setColourOpMultipassFallback(SceneBlendFactor sourceFactor, SceneBlendFactor destFactor)
    {
        mColourBlendFallbackSrc = sourceFactor;
        mColourBlendFallbackDest = destFactor;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setAlphaOperation(LayerBlendOperationEx op,
        LayerBlendSource source1,
        LayerBlendSource source2,
        Real arg1,
        Real arg2,
        Real manualBlend)
    {
        mAlphaBlendMode.operation = op;
        mAlphaBlendMode.source1 = source1;
        mAlphaBlendMode.source2 = source2;
        mAlphaBlendMode.alphaArg1 = arg1;
        mAlphaBlendMode.alphaArg2 = arg2;
        mAlphaBlendMode.factor = manualBlend;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::addEffect(TextureEffect& effect)
    {
        // Ensure controller pointer is null
        effect.controller = 0;

        if (effect.type == ET_ENVIRONMENT_MAP 
            || effect.type == ET_UVSCROLL
            || effect.type == ET_USCROLL
            || effect.type == ET_VSCROLL
            || effect.type == ET_ROTATE
            || effect.type == ET_PROJECTIVE_TEXTURE)
        {
            // Replace - must be unique
            // Search for existing effect of this type
            EffectMap::iterator i = mEffects.find(effect.type);
            if (i != mEffects.end())
            {
                // Destroy old effect controller if exist
                if (i->second.controller)
                {
                    ControllerManager::getSingleton().destroyController(i->second.controller);
                }

                mEffects.erase(i);
            }
        }

        if (isLoaded())
        {
            // Create controller
            createEffectController(effect);
        }

        // Record new effect
        mEffects.insert(EffectMap::value_type(effect.type, effect));

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::removeAllEffects(void)
    {
        // Iterate over effects to remove controllers
        EffectMap::iterator i, iend;
        iend = mEffects.end();
        for (i = mEffects.begin(); i != iend; ++i)
        {
            if (i->second.controller)
            {
                ControllerManager::getSingleton().destroyController(i->second.controller);
            }
        }

        mEffects.clear();
    }

    //-----------------------------------------------------------------------
    bool TextureUnitState::isBlank(void) const
    {
        if (mFramePtrs.empty())
            return true;
        else
            return !mFramePtrs[0]|| mTextureLoadFailed;
    }

    //-----------------------------------------------------------------------
    SceneBlendFactor TextureUnitState::getColourBlendFallbackSrc(void) const
    {
        return mColourBlendFallbackSrc;
    }
    //-----------------------------------------------------------------------
    SceneBlendFactor TextureUnitState::getColourBlendFallbackDest(void) const
    {
        return mColourBlendFallbackDest;
    }
    //-----------------------------------------------------------------------
    const LayerBlendModeEx& TextureUnitState::getColourBlendMode(void) const
    {
        return mColourBlendMode;
    }
    //-----------------------------------------------------------------------
    const LayerBlendModeEx& TextureUnitState::getAlphaBlendMode(void) const
    {
        return mAlphaBlendMode;
    }
    //-----------------------------------------------------------------------
    const TextureUnitState::UVWAddressingMode& 
    TextureUnitState::getTextureAddressingMode(void) const
    {
        return mAddressMode;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureAddressingMode(
        TextureUnitState::TextureAddressingMode tam)
    {
        mAddressMode.u = tam;
        mAddressMode.v = tam;
        mAddressMode.w = tam;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureAddressingMode(
        TextureUnitState::TextureAddressingMode u, 
        TextureUnitState::TextureAddressingMode v,
        TextureUnitState::TextureAddressingMode w)
    {
        mAddressMode.u = u;
        mAddressMode.v = v;
        mAddressMode.w = w;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureAddressingMode(
        const TextureUnitState::UVWAddressingMode& uvw)
    {
        mAddressMode = uvw;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureBorderColour(const ColourValue& colour)
    {
        mBorderColour = colour;
    }
    //-----------------------------------------------------------------------
    const ColourValue& TextureUnitState::getTextureBorderColour(void) const
    {
        return mBorderColour;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setEnvironmentMap(bool enable, EnvMapType envMapType)
    {
        if (enable)
        {
            TextureEffect eff;
            eff.type = ET_ENVIRONMENT_MAP;

            eff.subtype = envMapType;
            addEffect(eff);
        }
        else
        {
            removeEffect(ET_ENVIRONMENT_MAP);
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::removeEffect(TextureEffectType type)
    {
        // Get range of items matching this effect
        std::pair< EffectMap::iterator, EffectMap::iterator > remPair = 
            mEffects.equal_range( type );
        // Remove controllers
        for (EffectMap::iterator i = remPair.first; i != remPair.second; ++i)
        {
            if (i->second.controller)
            {
                ControllerManager::getSingleton().destroyController(i->second.controller);
            }
        }
        // Erase         
        mEffects.erase( remPair.first, remPair.second );
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setBlank(void)
    {
        mFramePtrs.clear();
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureTransform(const Matrix4& xform)
    {
        mTexModMatrix = xform;
        mRecalcTexMatrix = false;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureScroll(Real u, Real v)
    {
        mUMod = u;
        mVMod = v;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureScale(Real uScale, Real vScale)
    {
        mUScale = uScale;
        mVScale = vScale;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureRotate(const Radian& angle)
    {
        mRotate = angle;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    const Matrix4& TextureUnitState::getTextureTransform() const
    {
        if (mRecalcTexMatrix)
            recalcTextureMatrix();
        return mTexModMatrix;

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::recalcTextureMatrix() const
    {
        // Assumption: 2D texture coords
        Matrix4 xform;

        xform = Matrix4::IDENTITY;
        if (mUScale != 1 || mVScale != 1)
        {
            // Offset to center of texture
            xform[0][0] = 1/mUScale;
            xform[1][1] = 1/mVScale;
            // Skip matrix concat since first matrix update
            xform[0][3] = (-0.5f * xform[0][0]) + 0.5f;
            xform[1][3] = (-0.5f * xform[1][1]) + 0.5f;
        }

        if (mUMod || mVMod)
        {
            Matrix4 xlate = Matrix4::IDENTITY;

            xlate[0][3] = mUMod;
            xlate[1][3] = mVMod;

            xform = xlate * xform;
        }

        if (mRotate != Radian(0))
        {
            Matrix4 rot = Matrix4::IDENTITY;
            Radian theta ( mRotate );
            Real cosTheta = Math::Cos(theta);
            Real sinTheta = Math::Sin(theta);

            rot[0][0] = cosTheta;
            rot[0][1] = -sinTheta;
            rot[1][0] = sinTheta;
            rot[1][1] = cosTheta;
            // Offset center of rotation to center of texture
            rot[0][3] = 0.5f + ( (-0.5f * cosTheta) - (-0.5f * sinTheta) );
            rot[1][3] = 0.5f + ( (-0.5f * sinTheta) + (-0.5f * cosTheta) );

            xform = rot * xform;
        }

        mTexModMatrix = xform;
        mRecalcTexMatrix = false;

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureUScroll(Real value)
    {
        mUMod = value;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureVScroll(Real value)
    {
        mVMod = value;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureUScale(Real value)
    {
        mUScale = value;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureVScale(Real value)
    {
        mVScale = value;
        mRecalcTexMatrix = true;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setScrollAnimation(Real uSpeed, Real vSpeed)
    {
        // Remove existing effects
        removeEffect(ET_UVSCROLL);
        removeEffect(ET_USCROLL);
        removeEffect(ET_VSCROLL);

        // don't create an effect if the speeds are both 0
        if(uSpeed == 0.0f && vSpeed == 0.0f) 
        {
          return;
        }

        // Create new effect
        TextureEffect eff;
    if(uSpeed == vSpeed) 
    {
        eff.type = ET_UVSCROLL;
        eff.arg1 = uSpeed;
        addEffect(eff);
    }
    else
    {
        if(uSpeed)
        {
            eff.type = ET_USCROLL;
        eff.arg1 = uSpeed;
        addEffect(eff);
    }
        if(vSpeed)
        {
            eff.type = ET_VSCROLL;
            eff.arg1 = vSpeed;
            addEffect(eff);
        }
    }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setRotateAnimation(Real speed)
    {
        // Remove existing effect
        removeEffect(ET_ROTATE);
        // don't create an effect if the speed is 0
        if(speed == 0.0f) 
        {
          return;
        }
        // Create new effect
        TextureEffect eff;
        eff.type = ET_ROTATE;
        eff.arg1 = speed;
        addEffect(eff);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTransformAnimation(TextureTransformType ttype,
        WaveformType waveType, Real base, Real frequency, Real phase, Real amplitude)
    {
        // Remove existing effect
        // note, only remove for subtype, not entire ET_TRANSFORM
        // otherwise we won't be able to combine subtypes
        // Get range of items matching this effect
        for (EffectMap::iterator i = mEffects.begin(); i != mEffects.end(); ++i)
        {
            if (i->second.type == ET_TRANSFORM && i->second.subtype == ttype)
            {
                if (i->second.controller)
                {
                    ControllerManager::getSingleton().destroyController(i->second.controller);
                }
                mEffects.erase(i);

                // should only be one, so jump out
                break;
            }
        }

    // don't create an effect if the given values are all 0
    if(base == 0.0f && phase == 0.0f && frequency == 0.0f && amplitude == 0.0f) 
    {
      return;
    }
        // Create new effect
        TextureEffect eff;
        eff.type = ET_TRANSFORM;
        eff.subtype = ttype;
        eff.waveType = waveType;
        eff.base = base;
        eff.frequency = frequency;
        eff.phase = phase;
        eff.amplitude = amplitude;
        addEffect(eff);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_prepare(void)
    {
        // Unload first
        //_unload();

        // Load textures
        for (unsigned int i = 0; i < mFramePtrs.size(); ++i)
        {
            ensurePrepared(i);
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_load(void)
    {

        // Load textures
        for (unsigned int i = 0; i < mFramePtrs.size(); ++i)
        {
            ensureLoaded(i);
        }
        // Animation controller
        if (mAnimDuration != 0)
        {
            createAnimController();
        }
        // Effect controllers
        for (EffectMap::iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            createEffectController(it->second);
        }

    }
    //-----------------------------------------------------------------------
    const TexturePtr& TextureUnitState::_getTexturePtr(void) const
    {
        return _getTexturePtr(mCurrentFrame);
    }
    //-----------------------------------------------------------------------
    const TexturePtr& TextureUnitState::_getTexturePtr(size_t frame) const
    {
        if (frame < mFramePtrs.size())
        {
            if (mContentType == CONTENT_NAMED)
            {
                ensureLoaded(frame);
            }

            return mFramePtrs[frame];
        }
        
        // Silent fail with empty texture for internal method
        static TexturePtr nullTexPtr;
        return nullTexPtr;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_setTexturePtr(const TexturePtr& texptr)
    {
        _setTexturePtr(texptr, mCurrentFrame);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_setTexturePtr(const TexturePtr& texptr, size_t frame)
    {
        // generated textures (compositors & shadows) are only created at this point
        if(mFramePtrs.empty() && mContentType != CONTENT_NAMED)
            mFramePtrs.resize(1);

        assert(frame < mFramePtrs.size());
        mFramePtrs[frame] = texptr;
    }
    //-----------------------------------------------------------------------
    TexturePtr TextureUnitState::retrieveTexture(const String& name) {
        TextureManager::ResourceCreateOrRetrieveResult res;
        res = TextureManager::getSingleton().createOrRetrieve(name, mParent->getResourceGroup());
        return static_pointer_cast<Texture>(res.first);
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::ensurePrepared(size_t frame) const
    {
        const TexturePtr& tex = mFramePtrs[frame];
        if (!tex || mTextureLoadFailed)
            return;

        tex->setGamma(mGamma);

        try {
            tex->prepare();
        }
        catch (Exception& e)
        {
            String msg = "preparing texture '" + tex->getName() +
                         "'. Texture layer will be blank: " + e.getFullDescription();
            LogManager::getSingleton().logError(msg);
            mTextureLoadFailed = true;
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::ensureLoaded(size_t frame) const
    {
        const TexturePtr& tex = mFramePtrs[frame];
        if (!tex || mTextureLoadFailed)
            return;

        tex->setGamma(mGamma);

        try {
            tex->load();
        }
        catch (Exception& e)
        {
            String msg = "loading texture '" + tex->getName() +
                         "'. Texture layer will be blank: " + e.getFullDescription();
            LogManager::getSingleton().logError(msg);
            mTextureLoadFailed = true;
        }
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::createAnimController(void)
    {
        if (mAnimController)
        {
            ControllerManager::getSingleton().destroyController(mAnimController);
            mAnimController = 0;
        }
        mAnimController = ControllerManager::getSingleton().createTextureAnimator(this, mAnimDuration);

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::createEffectController(TextureEffect& effect)
    {
        if (effect.controller)
        {
            ControllerManager::getSingleton().destroyController(effect.controller);
            effect.controller = 0;
        }
        ControllerManager& cMgr = ControllerManager::getSingleton();
        switch (effect.type)
        {
        case ET_UVSCROLL:
            effect.controller = cMgr.createTextureUVScroller(this, effect.arg1);
            break;
        case ET_USCROLL:
            effect.controller = cMgr.createTextureUScroller(this, effect.arg1);
            break;
        case ET_VSCROLL:
            effect.controller = cMgr.createTextureVScroller(this, effect.arg1);
            break;
        case ET_ROTATE:
            effect.controller = cMgr.createTextureRotater(this, effect.arg1);
            break;
        case ET_TRANSFORM:
            effect.controller = cMgr.createTextureWaveTransformer(this, (TextureUnitState::TextureTransformType)effect.subtype, effect.waveType, effect.base,
                effect.frequency, effect.phase, effect.amplitude);
            break;
        case ET_ENVIRONMENT_MAP:
            break;
        default:
            break;
        }
    }
    //-----------------------------------------------------------------------
    Real TextureUnitState::getTextureUScroll(void) const
    {
        return mUMod;
    }

    //-----------------------------------------------------------------------
    Real TextureUnitState::getTextureVScroll(void) const
    {
        return mVMod;
    }

    //-----------------------------------------------------------------------
    Real TextureUnitState::getTextureUScale(void) const
    {
        return mUScale;
    }

    //-----------------------------------------------------------------------
    Real TextureUnitState::getTextureVScale(void) const
    {
        return mVScale;
    }

    //-----------------------------------------------------------------------
    const Radian& TextureUnitState::getTextureRotate(void) const
    {
        return mRotate;
    }
    
    //-----------------------------------------------------------------------
    Real TextureUnitState::getAnimationDuration(void) const
    {
        return mAnimDuration;
    }

    //-----------------------------------------------------------------------
    const TextureUnitState::EffectMap& TextureUnitState::getEffects(void) const
    {
        return mEffects;
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureFiltering(TextureFilterOptions filterType)
    {
        switch (filterType)
        {
        case TFO_NONE:
            setTextureFiltering(FO_POINT, FO_POINT, FO_NONE);
            break;
        case TFO_BILINEAR:
            setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_POINT);
            break;
        case TFO_TRILINEAR:
            setTextureFiltering(FO_LINEAR, FO_LINEAR, FO_LINEAR);
            break;
        case TFO_ANISOTROPIC:
            setTextureFiltering(FO_ANISOTROPIC, FO_ANISOTROPIC, Root::getSingleton().getRenderSystem()->hasAnisotropicMipMapFilter() ? FO_ANISOTROPIC : FO_LINEAR);
            break;
        }
        mIsDefaultFiltering = false;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureFiltering(FilterType ft, FilterOptions fo)
    {
        switch (ft)
        {
        case FT_MIN:
            mMinFilter = fo;
            break;
        case FT_MAG:
            mMagFilter = fo;
            break;
        case FT_MIP:
            mMipFilter = fo;
            break;
        }
        mIsDefaultFiltering = false;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureFiltering(FilterOptions minFilter, 
        FilterOptions magFilter, FilterOptions mipFilter)
    {
        mMinFilter = minFilter;
        mMagFilter = magFilter;
        mMipFilter = mipFilter;
        mIsDefaultFiltering = false;
    }
    //-----------------------------------------------------------------------
    FilterOptions TextureUnitState::getTextureFiltering(FilterType ft) const
    {
        if(mIsDefaultFiltering)
            return MaterialManager::getSingleton().getDefaultTextureFiltering(ft);

        switch (ft)
        {
        case FT_MIN:
            return mMinFilter;
        case FT_MAG:
            return mMagFilter;
        case FT_MIP:
            return mMipFilter;
        }
        // to keep compiler happy
        return mMinFilter;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureCompareEnabled(bool enabled)
    {
        mCompareEnabled=enabled;
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::getTextureCompareEnabled() const
    {
        return mCompareEnabled;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureCompareFunction(CompareFunction function)
    {
        mCompareFunc=function;
    }
    //-----------------------------------------------------------------------
    CompareFunction TextureUnitState::getTextureCompareFunction() const
    {
        return mCompareFunc;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureAnisotropy(unsigned int maxAniso)
    {
        mMaxAniso = maxAniso;
        mIsDefaultAniso = false;
    }
    //-----------------------------------------------------------------------
    unsigned int TextureUnitState::getTextureAnisotropy() const
    {
        return mIsDefaultAniso? MaterialManager::getSingleton().getDefaultAnisotropy() : mMaxAniso;
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::_unprepare(void)
    {
        // don't unload textures. may be used elsewhere
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_unload(void)
    {
        // Destroy animation controller
        if (mAnimController)
        {
            ControllerManager::getSingleton().destroyController(mAnimController);
            mAnimController = 0;
        }

        // Destroy effect controllers
        for (EffectMap::iterator i = mEffects.begin(); i != mEffects.end(); ++i)
        {
            if (i->second.controller)
            {
                ControllerManager::getSingleton().destroyController(i->second.controller);
                i->second.controller = 0;
            }
        }

        // don't unload textures. may be used elsewhere
    }
    //-----------------------------------------------------------------------------
    bool TextureUnitState::isLoaded(void) const
    {
        return mParent->isLoaded();
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::_notifyNeedsRecompile(void)
    {
        mParent->_notifyNeedsRecompile();
    }
    //-----------------------------------------------------------------------
    bool TextureUnitState::hasViewRelativeTextureCoordinateGeneration(void) const
    {
        // Right now this only returns true for reflection maps

        EffectMap::const_iterator i, iend;
        iend = mEffects.end();
        
        for(i = mEffects.find(ET_ENVIRONMENT_MAP); i != iend; ++i)
        {
            if (i->second.subtype == ENV_REFLECTION)
                return true;
        }

        if(mEffects.find(ET_PROJECTIVE_TEXTURE) != iend)
        {
            return true;
        }

        return false;
    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setProjectiveTexturing(bool enable, 
        const Frustum* projectionSettings)
    {
        if (enable)
        {
            TextureEffect eff;
            eff.type = ET_PROJECTIVE_TEXTURE;
            eff.frustum = projectionSettings;
            addEffect(eff);
        }
        else
        {
            removeEffect(ET_PROJECTIVE_TEXTURE);
        }

    }
    //-----------------------------------------------------------------------
    void TextureUnitState::setName(const String& name)
    {
        mName = name;
        if (mTextureNameAlias.empty())
            mTextureNameAlias = mName;
    }

    //-----------------------------------------------------------------------
    void TextureUnitState::setTextureNameAlias(const String& name)
    {
        mTextureNameAlias = name;
    }

    //-----------------------------------------------------------------------
    bool TextureUnitState::applyTextureAliases(const AliasTextureNamePairList& aliasList, const bool apply)
    {
        bool testResult = false;
        // if TUS has an alias see if its in the alias container
        if (!mTextureNameAlias.empty())
        {
            AliasTextureNamePairList::const_iterator aliasEntry =
                aliasList.find(mTextureNameAlias);

            if (aliasEntry != aliasList.end())
            {
                // match was found so change the texture name in mFrames
                testResult = true;

                if (apply)
                {
                    // currently assumes animated frames are sequentially numbered
                    // cubic, 1d, 2d, and 3d textures are determined from current TUS state
                    
                    // if cubic or 3D
                    if (mCubic)
                    {
                        setCubicTextureName(aliasEntry->second, getTextureType() == TEX_TYPE_CUBE_MAP);
                    }
                    else
                    {
                        // if more than one frame then assume animated frames
                        if (mFramePtrs.size() > 1)
                            setAnimatedTextureName(aliasEntry->second, 
                                static_cast<unsigned int>(mFramePtrs.size()), mAnimDuration);
                        else
                            setTextureName(aliasEntry->second, getTextureType());
                    }
                }
                
            }
        }

        return testResult;
    }
    //-----------------------------------------------------------------------------
    void TextureUnitState::_notifyParent(Pass* parent)
    {
        mParent = parent;
    }
    //-----------------------------------------------------------------------------
    void TextureUnitState::setCompositorReference(const String& compositorName, const String& textureName, size_t mrtIndex)
    {  
        mCompositorRefName = compositorName; 
        mCompositorRefTexName = textureName; 
        mCompositorRefMrtIndex = mrtIndex; 
    }
    //-----------------------------------------------------------------------
    size_t TextureUnitState::calculateSize(void) const
    {
        size_t memSize = 0;

        memSize += sizeof(unsigned int) * 3;
        memSize += sizeof(int);
        memSize += sizeof(float);
        memSize += sizeof(Real) * 5;
        memSize += sizeof(bool) * 8;
        memSize += sizeof(size_t);
        memSize += sizeof(TextureType);
        memSize += sizeof(PixelFormat);
        memSize += sizeof(UVWAddressingMode);
        memSize += sizeof(ColourValue);
        memSize += sizeof(LayerBlendModeEx) * 2;
        memSize += sizeof(SceneBlendFactor) * 2;
        memSize += sizeof(Radian);
        memSize += sizeof(Matrix4);
        memSize += sizeof(FilterOptions) * 3;
        memSize += sizeof(CompareFunction);
        memSize += sizeof(BindingType);
        memSize += sizeof(ContentType);
        memSize += sizeof(String) * 4;

        memSize += mFramePtrs.size() * sizeof(TexturePtr);
        memSize += mEffects.size() * sizeof(TextureEffect);

        return memSize;
    }
}
