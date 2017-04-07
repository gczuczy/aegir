import { AegirUiPage } from './app.po';

describe('aegir-ui App', () => {
  let page: AegirUiPage;

  beforeEach(() => {
    page = new AegirUiPage();
  });

  it('should display message saying app works', () => {
    page.navigateTo();
    expect(page.getParagraphText()).toEqual('app works!');
  });
});
