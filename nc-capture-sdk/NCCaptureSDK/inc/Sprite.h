#pragma once
#include "common.h"
#include <SpriteBatch.h>
#include <SimpleMath.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <wrl.h>
#include "NCCapture.h"


using namespace DirectX;
// sprite for game screen 
class NCCAPTURESDK_API GameSprite
{
public:
	GameSprite();
	~GameSprite();

	bool		Create(const wchar_t* filename);

	void		Draw(const SIZE& gameSize, const FRECT& dest, DirectX::FXMVECTOR color = DirectX::Colors::White);
private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_srv;
};

// game capture sprite
class NCCAPTURESDK_API GameCaptureSprite
{
public:
	GameCaptureSprite();
	~GameCaptureSprite();

//	bool		Create(ID3D11Texture2D* texture);
//	bool		Create(CapturedImage* pImage);
	bool		Create(ID3D11ShaderResourceView* srv, bool flip=false);

	void		UpdateTexture(ID3D11Texture2D* texture);
//	void		UpdateTexture(DirectX::Image* pImage);

	void		DrawFull(FXMVECTOR color = Colors::White);
	void		Draw(int x, int y, FXMVECTOR color = Colors::White);
	void		Draw(const RECT& dest, FXMVECTOR color = Colors::White);

	int			GetWidth() { return m_textureWidth; }
	int			GetHeight() { return m_textureHeight; }

private:
	int                                                 m_textureWidth;
	int                                                 m_textureHeight;
	DirectX::SimpleMath::Vector2						m_screenPos;
	DirectX::SimpleMath::Vector2						m_textureSize;
	DirectX::SimpleMath::Vector2						m_origin;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>    m_srv;// 연결된 D3D11Texture2D는 NCCaptureSDK가 가지고 있다.
	Microsoft::WRL::ComPtr<ID3D11Texture2D>				m_texture;
	bool												m_flip = false;
};

