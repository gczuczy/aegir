import { ComponentFixture, TestBed } from '@angular/core/testing';

import { AddProgramComponent } from './add-program.component';

describe('AddProgramComponent', () => {
  let component: AddProgramComponent;
  let fixture: ComponentFixture<AddProgramComponent>;

  beforeEach(() => {
    TestBed.configureTestingModule({
      declarations: [AddProgramComponent]
    });
    fixture = TestBed.createComponent(AddProgramComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
