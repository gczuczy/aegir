import { ComponentFixture, TestBed } from '@angular/core/testing';

import { LoadProgramComponent } from './load-program.component';

describe('LoadProgramComponent', () => {
  let component: LoadProgramComponent;
  let fixture: ComponentFixture<LoadProgramComponent>;

  beforeEach(() => {
    TestBed.configureTestingModule({
      declarations: [LoadProgramComponent]
    });
    fixture = TestBed.createComponent(LoadProgramComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
