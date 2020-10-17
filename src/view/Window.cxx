#include "Window.hxx"
#include "../renderer/SDLRenderer.hxx"
#include "../util/LOG.hxx"
#include "../util/Filesystem.hxx"
#include "../GameService.hxx"
#include "../engine/basics/Settings.hxx"

using namespace std::chrono_literals;

Window::Window(GameService::ServiceTuple context, const char * title, unsigned int width, unsigned int height, bool isFullScreen, const string & windowIcon) :
  m_MouseState(*std::get<UILoopMQ*>(context)),
  m_Cursor(new CursorView)
{
  Uint32 windowFlags = isFullScreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE;
#ifdef __ANDROID__
  // Android is always fullscreen.. We also need to set screenWidth / screenHeight to the max. resolution in Fullscreen
  windowFlags = SDL_WINDOW_FULLSCREEN;
  SDL_DisplayMode mode;
  SDL_GetDesktopDisplayMode(0, &mode);
  width = mode.w;
  height = mode.h;
#endif
  
  windowFlags |= SDL_WINDOW_INPUT_GRABBED;
  m_Window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
      width, height, windowFlags);
  if (!m_Window)
    throw UIError(TRACE_INFO "Failed to create window: " + string{SDL_GetError()});
  SDL_RaiseWindow(m_Window);

  m_Renderer = std::make_unique<SDLRenderer>(m_Window);
  string iconFName = fs::getBasePath() + windowIcon;
  PixelBuffer iconPB = fs::readPNGFile(iconFName);
  SDL_Surface* icon = iconPB.toSurface();
  
  if (!icon)
    throw UIError(TRACE_INFO "Could not load icon " + iconFName + ": " + SDL_GetError());

  SDL_SetWindowIcon(m_Window, icon);
  SDL_FreeSurface(icon);
  m_MouseState.addObserver(m_Cursor);
  m_MouseState.setCursor(CursorType::Arrow);
}

Window::~Window()
{
  /* This will destroy the renderer first (required for SDLRenderer) */
  LOG(LOG_WARNING) << "Destroying renderer...";
  m_Renderer = nullptr;
  LOG(LOG_WARNING) << "Destroying window...";
  SDL_DestroyWindow(m_Window);
}

void Window::setActivity(iActivityPtr activity)
{
  m_Renderer->clear();
  std::swap(m_Activity, activity);
  LOG(LOG_DEBUG) << "Setting up new activity";
  m_Activity->setup(*m_Activity);
  LOG(LOG_DEBUG) << "Drawing new activity";
  m_Activity->draw(*m_Renderer);
  m_Renderer->commit();
}

Rectangle Window::getBounds() const noexcept
{
  return m_Renderer->getDrawableSize();
}

void Window::handleEvent(WindowResizeEvent &&event)
{
  m_Renderer->clear();
  m_Activity->setBounds(getBounds());
  m_Activity->setup(*m_Activity);
  m_Activity->draw(*m_Renderer);
  m_Renderer->commit();
}

void Window::handleEvent(WindowRedrawEvent &&event)
{
  m_Renderer->clear();
  m_Activity->draw(*m_Renderer);
  m_Renderer->commit();
}
